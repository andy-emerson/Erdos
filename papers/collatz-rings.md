# Collatz maps across rings of integers

*Draft, 5 November 2026. Code, logs and Lean proofs: `research/collatz/`. Items marked [unverified] have not been checked against their source. The companion paper [Paper 2] cites this one for its definitions and for the results of Sections 2, 3 and 6.*

## Abstract

The $3x+1$ map lives on the positive integers. We place it in a family of maps over a ring of integers $R$: a map multiplies an odd element by a fixed $\alpha$, adds a digit chosen by the class of the element modulo $\pi^k$ for a prime $\pi$ above $2$, and divides by $\pi$ while it can. A Collatz map is one in which every nonzero orbit reaches a cycle through a unit, with no other cycle and no divergent orbit. We climb from $\mathbb{N}$ to $\mathbb{Z}$, to the Gaussian integers $\mathbb{Z}[i]$ and to the Eisenstein integers $\mathbb{Z}[\omega]$, and at each step ask what persists, what stops mattering and what becomes important. The window $q^k < N(\alpha) < q^{k + 1/(q-1)}$, where $q$ is the size of the residue field, persists in every ring, and so does the mechanism that makes a map have many cycles. "Reaching $1$" becomes "reaching a unit". On $\mathbb{Z}$ the unit $-1$ becomes important: $3x \pm 1$ fail there, and from modulus $4$ on the smallest digits give maps that commute with $-1$, among them Barina's $7x \pm 1$. On $\mathbb{Z}[i]$ the modulus $1+i$ of $3x+1$ is impossible, Collatz maps appear at the next modulus, $(1+i)^2$, and none of them commutes with the units. Commuting with the units is not needed for being a Collatz map, but without it the rotated and mirrored maps are unrelated and the units no longer behave alike. We prove that maps commuting with a finite unit group exist exactly from a threshold modulus, and that none exist when the unit group is infinite.

## 1. Introduction

### 1.1 The lineage

The $3x+1$ map sends an even positive integer $x$ to $x/2$ and an odd one to $3x+1$. The Collatz conjecture says that every positive integer reaches $1$. Collatz says he invented the problem before 1952 and passed it to Hasse in 1952, and Coxeter gave the first published statement in 1971, with a prize of fifty dollars for a proof and one hundred for a counterexample [Lagarias 2016, p. 772; Collatz 1986; Coxeter 1971]. Lagarias's survey [Lagarias 1985], his overview [Lagarias 2010] and his two annotated bibliographies [Lagarias 2003; Lagarias 2006] record the problem and its generalizations.

The same rule makes sense on $\mathbb{Z}$, where it has more cycles. Besides $0$ and $1 \to 4 \to 2$ it has the cycle $-1 \to -2$, a cycle of $5$ steps through $-5$ and one of $18$ steps through $-17$, and the conjecture of Böhm and Sontacchi says that these are all [Lagarias 1985]. Since $-(3x+1) = 3(-x) - 1$, the map acts on the negative integers as $3x - 1$ acts on the positive ones, and the problem of $3x-1$ on $\mathbb{N}$ is the problem of $3x+1$ on the negative integers [OEIS A037084].

Hasse proposed a class of generalized maps: multiply by $m$, subtract the residue that makes the result divisible by $d$, and divide [Hasse 1975; Lagarias 2003]. Möller [Möller 1978] and Heppner [Heppner 1978] studied it, and Matthews and Watts [Matthews and Watts 1984] extended it to maps $x \mapsto (m_i x - r_i)/d$ on the classes $i$ modulo $d$, with a conjecture that almost all trajectories cycle if $|m_0 \cdots m_{d-1}| < d^d$ and almost all diverge if the product exceeds $d^d$ [Matthews 2010, Conjecture 3.1]. Matthews derives this from a growth-rate argument [Matthews 2010, Section 3.2], and the upper end of the window of Section 2 is that argument carried over to the norm. Conway proved that a generalization of $3x+1$ to residue-class-wise affine maps is undecidable [Conway 1972], so maps of this kind have to be studied one at a time. Matthews's chapter also treats maps on the ring of integers of a number field, modulo a non-unit, with worked examples on $\mathbb{Z}[\sqrt 2]$ and $\mathbb{Z}[\sqrt 3]$, and closes with the remark that rings other than $\mathbb{Z}$ invite systematic investigation [Matthews 2010, Section 10]. Leigh is reported to have been the first to consider such maps on rings of algebraic integers [Siegel 2021; unverified: Leigh's work not seen].

Barina's $7x \pm 1$ map [Barina 2018] sends an even $n$ to $n/2$, an $n \equiv 1 \pmod 4$ to $7n+1$ and an $n \equiv 3$ to $7n-1$. He conjectured that every positive orbit reaches $1$, verified this up to $10^{15}$, gave the heuristic $\frac{1}{2}\log\frac{7}{4} + \frac{1}{2}\log\frac{1}{2} < 0$, and noted that the orbits of negative numbers are the negatives of those of positive numbers, in contrast with the negative cycles of $3x+1$. The map is in the OEIS [OEIS A317640], with entries for its stopping times and shortcut form [OEIS A317753; OEIS A318489; OEIS A318972], and Matthews has a page on it [Matthews, Barina's 7x+1 conjecture; unverified: the page itself was not reached]. Negative multipliers appear in the OEIS too: del Arte's sequence [OEIS A305057] iterates $x \mapsto -5x + 1$ on odd $x$ and $x \mapsto x/2$ on even $x$, starting at $5$.

On the Gaussian integers, Joseph [Joseph 1998] extended $3x+1$ to $\mathbb{Z}_2[i]$ by $z \mapsto z/2$ on the class $0$ modulo $2$ and $z \mapsto (3z+\beta)/2$ on the class $\beta \in \{1, i, 1+i\}$. Kucinski [Kucinski 2000] studied this map on $\mathbb{Z}[i]$ and found $77$ cycles of period at most $400$ [unverified: read through a text extraction], and Chamberland's survey records both [Chamberland]. Alvarado and the students of a 2017 REU at CSU Channel Islands studied the map that sends $z$ to $z/(1+i)$ when $1+i$ divides $z$ and to $(2+i)z + u$ otherwise, with $u$ a fixed unit, and found that about $5.6\%$ of the starts with $|a|, |b| \le 100$ reached a unit [Alvarado 2025; unverified: talk slides only]. Kionke [Kionke 2017] and Siegel [Siegel 2024] give frameworks on lattices and on rings of integers.

This paper takes the ring as a parameter. It defines one family of maps over $\mathbb{Z}$, $\mathbb{Z}[i]$ and $\mathbb{Z}[\omega]$, with the modulus and the digit table as parameters, and follows the family up the rings. As far as our search reached, no earlier work does this [unverified: parts of the bibliographies and several of Matthews's web pages could not be read].

### 1.2 The question

Each ring contains the previous one, and each step adds units: $\mathbb{N}$ has the unit $1$ only, $\mathbb{Z}$ has $\pm 1$, $\mathbb{Z}[i]$ has four units and $\mathbb{Z}[\omega]$ six. At each step we ask three questions. What persists from the smaller ring? What stops mattering? What becomes important, including properties that cannot be asked about in $\mathbb{N}$?

Section 2 sets up the family and the window for all three rings. Sections 3, 4 and 5 take the steps from $\mathbb{N}$ to $\mathbb{Z}$, from $\mathbb{Z}$ to $\mathbb{Z}[i]$ and from $\mathbb{Z}[i]$ to $\mathbb{Z}[\omega]$. One of the things that become important is whether a map commutes with the units. In $\mathbb{Z}$ it comes free with the simplest maps; in $\mathbb{Z}[i]$ it does not, and Section 6 shows when such maps exist and what they gain. We observe this property and show that it matters, without exploring it in depth.

### 1.3 Glossary

These are the paper's terms in brief. Section 2 gives the formal definitions.

- A **map** is a generalized Collatz map: a multiplier $\alpha$, a modulus $\pi^k$ for a prime $\pi$ above $2$, and a digit table, which gives for each odd class modulo $\pi^k$ the number added after multiplying. The map $T$ divides an element by $\pi$ when it can and otherwise multiplies by $\alpha$ and adds the digit. The accelerated map $S$ does one multiplication and all the divisions that follow.
- An element is **odd** if $\pi$ does not divide it.
- A **trajectory** or **orbit** is the sequence of iterates of one start.
- A **trivial cycle** is a cycle that contains a unit. A **unit cycle** is a cycle made only of units.
- A **Collatz map** is a map in which every nonzero orbit reaches a trivial cycle. It has no other cycle and no divergent orbit.
- A map **commutes with the units** if $T(uz) = uT(z)$ for every unit $u$. In $\mathbb{Z}$ this says that $T$ is an odd function.
- The **rotated** maps of a map replace $\alpha$ by $u\alpha$ and the **mirrored** map replaces $\alpha$ by its complex conjugate, each with the digit table carried along.
- The **window** is the range of multipliers for which a map expands in the worst case and contracts on average.
- The **drift** is the mean change of $\log_2$ of the norm per odd step, in a model in which the orbit visits the residue classes at random.
- The **score** is $1/M - 1/N$, where $N$ is the norm of the multiplier and $M$ the norm of the modulus.

## 2. Maps over a ring of integers

### 2.1 The rings

We work in three rings of integers, $\mathbb{Z}$, $\mathbb{Z}[i]$ and $\mathbb{Z}[\omega]$ with $\omega = e^{2\pi i/3}$, and use the semiring $\mathbb{N}$ of positive integers as the starting point. Each ring has a finite group of units $U$ and an absolute norm $N$, with $N(xy) = N(x)N(y)$ and $N(u) = 1$ exactly for units: $N(x) = |x|$ on $\mathbb{Z}$, $N(a+bi) = a^2 + b^2$ on $\mathbb{Z}[i]$, and $N(a + b\omega) = a^2 - ab + b^2$ on $\mathbb{Z}[\omega]$. In each ring we fix one prime $\pi$ above $2$, and write $q = N(\pi)$ for the size of the residue field $R/(\pi)$.

| ring | units | $\pi$ | $q$ | $2$ in $R$ |
|---|---|---|---|---|
| $\mathbb{Z}$ | $\pm 1$ | $2$ | $2$ | prime |
| $\mathbb{Z}[i]$ | $\pm 1, \pm i$ | $1+i$ | $2$ | ramified, $2 = -i(1+i)^2$ |
| $\mathbb{Z}[\omega]$ | $\pm 1, \pm\omega, \pm\omega^2$ | $2$ | $4$ | inert |

An element $z$ is odd if $\pi \nmid z$ and even otherwise. In $\mathbb{Z}[i]$, $a+bi$ is even exactly when $a+b$ is even, and $(a+bi)/(1+i) = ((a+b) + (b-a)i)/2$. Dividing by $\pi$ divides the norm by $q$.

*Definition 2.1 (map).* A map is a triple $(\alpha, k, d)$ with $\alpha$ odd, $k \ge 0$, and a digit table $d$: a function on the odd elements that depends only on the class modulo $\pi^k$, the modulus, and satisfies $\pi^k \mid \alpha z + d(z)$. The digit set is $D = \{d(z) : z \text{ odd}\}$. The map is
$$T(z) = \begin{cases} z/\pi & \text{if } \pi \mid z, \\ \alpha z + d(z) & \text{otherwise.} \end{cases}$$
For odd $z$ with $w = \alpha z + d(z) \ne 0$, the accelerated map is $S(z) = w/\pi^{v(w)}$, where $v(w)$ is the exponent of $\pi$ in $w$, so that $S(z)$ is odd and $v(w) \ge k$. If $w = 0$, $S(z) = 0$ and the orbit ends at $0$.

The congruence $d(z) \equiv -\alpha z \pmod{\pi^k}$ depends only on the class of $z$, and since $\alpha$ is invertible modulo $\pi^k$, the map $z \mapsto -\alpha z$ permutes the odd classes. There are $q^{k-1}(q-1)$ odd classes for $k \ge 1$. In $\mathbb{Z}$, $3x+1$ is the map $(3, 1, d)$ with $d = 1$, and $7x \pm 1$ is $(7, 2, d)$ with $d(x) = 1$ for $x \equiv 1$ and $d(x) = -1$ for $x \equiv 3 \pmod 4$.

*Lemma 2.2.* Let $k \ge 1$. Every digit is odd, and $D$ has exactly one element in each odd class modulo $\pi^k$.

*Proof.* $d(z) \equiv -\alpha z \pmod \pi$, which is odd because $\pi$ is prime. If $d(z) \equiv d(z') \pmod{\pi^k}$, then $\pi^k$ divides $(\alpha z' + d(z')) - (\alpha z + d(z))$ and $d(z') - d(z)$, hence $\alpha(z' - z)$. Since $\alpha$ is odd and $\pi$ is prime, $\pi^k \mid z' - z$, so $z$ and $z'$ lie in the same class and $d(z) = d(z')$. $\square$

So for $k \ge 1$, $d(z)$ is the element of $D$ in the class of $-\alpha z$, and a map is the same as a choice of one odd element in each odd class. For $\mathbb{Z}[i]$ the lemma is proved in Lean (`Expr.d_odd`, `Expr.d_inj`, Section 9).

*Definition 2.3 (cycles and Collatz maps).* A point $x$ is periodic for a function $f$ if $f^p(x) = x$ for some $p \ge 1$, and its cycle is the set of its iterates. A cycle is trivial if it contains a unit, and a unit cycle if all its elements are units. A map is a Collatz map if every odd $z$ has an iterate under $S$ that is periodic with a trivial cycle.

Cycles are taken literally, as sets of elements. A Collatz map has no cycle without a unit and no divergent orbit. Triviality can be read off $T$ or $S$: a nonzero even element reaches its odd part under $T$, between consecutive odd terms of a $T$-orbit there are only even elements, and units are odd. On $\mathbb{N}$, whose only unit is $1$, the definition says that every orbit reaches the cycle through $1$, which is the Collatz conjecture for $3x+1$.

### 2.2 The window

Let $|z|$ be the complex absolute value, so that $N(z) = |z|$ on $\mathbb{Z}$ and $N(z) = |z|^2$ on the two quadratic rings. An odd step multiplies $|z|$ by about $|\alpha|$, and at least $k$ divisions by $\pi$ follow.

*Proposition 2.4 (below the window).* If $N(\alpha) < q^k$, every orbit of $T$ is eventually periodic.

*Proof.* Let $\rho = |\alpha|/|\pi|^k < 1$ and $\delta = \max_{d \in D} |d|$. For odd $z$, $|S(z)| \le \rho|z| + \delta/|\pi|^k$. With $R_0 = \delta/(|\pi|^k(1 - \rho))$, $|S(z)| < |z|$ when $|z| > R_0$, and $|S(z)| \le R_0$ when $|z| \le R_0$. Even steps lower $|z|$. Norms are nonnegative integers, so every orbit enters the disc $|z| \le R_0$ and stays bounded, and a bounded set of elements of $R$ is finite. $\square$

Such maps are settled by a finite computation. In $\mathbb{Z}[i]$, $\alpha = 1+2i$ at modulus $(1+i)^3$ with the units as digits is an example: every odd step from a non-unit lowers the norm, and the units form one cycle $1 \to -i \to -1 \to i \to 1$ (proved; [Paper 2] gives the details). It plays the part of the $x+1$ problem, which Lagarias calls the solved cousin of $3x+1$ [Lagarias 2016].

If $N(\alpha) > q^k$, an odd step with exactly $k$ divisions multiplies the norm by $N(\alpha)/q^k > 1$, so the map expands in the worst case. On average there are more divisions.

*Proposition 2.5 (valuations in density).* Fix an odd class $c$ modulo $\pi^k$ and $j \ge 0$. As $z$ runs over the odd classes modulo $\pi^{k+j}$ that lie in $c$, the fraction with $v(\alpha z + d(z)) \ge k + j$ is $q^{-j}$. So in density over odd $z$, $v - k$ has $P(v - k \ge j) = q^{-j}$ and mean $1/(q-1)$.

*Proof.* Write $z = z_0 + \pi^k t$ with $t$ running over $R$ modulo $\pi^j$. The digit is that of $c$, so $\alpha z + d = (\alpha z_0 + d) + \alpha\pi^k t$. Since $\alpha$ is invertible modulo $\pi^j$, this runs once over the $q^j$ classes of multiples of $\pi^k$ modulo $\pi^{k+j}$, and one of them is the class of $0$. The mean is $\sum_{j \ge 1} q^{-j} = 1/(q-1)$. $\square$

If an orbit visits the classes as a random element would, the mean change of $\log_2 N$ per odd step is
$$\log_2 N(\alpha) - (k + \frac{1}{q-1})\log_2 q,$$
which is negative exactly when $N(\alpha) < q^{k + 1/(q-1)}$. Proposition 2.5 is a statement about density, and its use for orbits is a heuristic, as it is for $3x+1$.

*Definition 2.6 (window).* A map is in the window if
$$q^k < N(\alpha) < q^{k + 1/(q-1)}.$$

For $q = 2$ this is $2^k < N(\alpha) < 2^{k+1}$. For $3x+1$ it reads $2 < 3 < 4$ at modulus $2$, and for Barina's map $4 < 7 < 8$ at modulus $4$. In $\mathbb{Z}[\omega]$, $q = 4$ and the window is $(4^k, 4^{k+1/3})$, not $(4^k, 4^{k+1})$, because a division by $2$ divides the norm by $4$ and the mean excess of divisions is $1/3$. The upper end is the growth-rate heuristic of Matthews and Watts carried over to the norm. At $k = 1$ it is $q^{q/(q-1)}$, the form of Heppner's threshold $m < d^{d/(d-1)}$ for Hasse's maps, below which almost all iterates get smaller [Heppner 1978, as annotated in Lagarias 2003; unverified: not seen]. The windows for different $k$ are disjoint, so in a given ring the norm of a multiplier in the window determines its modulus.

### 2.3 Score and drift

*Definition 2.7.* For a map with multiplier norm $N = N(\alpha)$ and modulus norm $M = N(\pi^k) = q^k$, the score is
$$\frac{1}{M} - \frac{1}{N} = \frac{N - M}{NM},$$
and the drift is
$$\log_2 \frac{N}{M} - \frac{\log_2 q}{q-1},$$
the mean change of $\log_2 N(z)$ per odd step from Section 2.2.

Inside the window the score lies in $(0, 1/M - q^{-k-1/(q-1)})$, which for $q = 2$ is $(0, 1/(2M))$, and the drift lies in $(-\log_2 q/(q-1), 0)$, which for $q = 2$ is $(-1, 0)$. Below the window the score is negative. Within one modulus the score and the drift order maps alike, since both increase with $N$. Across moduli the score favours the smaller modulus through the term $1/M$, while the drift ignores the size of the modulus. We compare scores within one ring. In $\mathbb{Z}$ the norm is $|x|$, so the drift is in bits of $|x|$; for $3x+1$ on $\mathbb{N}$ the score is $1/2 - 1/3 = 0.167$ and the drift $\log_2 3 - 2 = -0.415$ bits per odd step. We have not searched for prior uses of such a score.

## 3. From $\mathbb{N}$ to $\mathbb{Z}$

### 3.1 What persists

The window persists, and with it the maps of $\mathbb{N}$. A map on $\mathbb{Z}$ with $m > 0$ and positive digits sends positive integers to positive integers, so the question of $\mathbb{N}$ can still be asked of it. Call the digit set $\{1, 3, \ldots, 2^k - 1\}$ of least positive residues, the one $3x+1$ uses, the positive digit set. For $2^k < m < \frac{5}{3}2^k$ the maps with positive digits at moduli $4$ to $1024$ were run on all odd $|x| \le 10^5$, and the number with a single cycle on the positive integers is $0, 1, 2, 3, 5, 15, 33, 55, 112$ by modulus (computed to $10^5$). The smallest multiplier $2^k + 1$ has a single positive cycle at every modulus from $8$ to $1024$.

The mechanism that gives a map many cycles also persists. Let a cycle have $n$ odd steps $z_{j+1} = (\alpha z_j + d_j)/\pi^{v_j}$, total exponent $V = v_0 + \cdots + v_{n-1}$, and a unit $\varepsilon$ with $S^n(z_0) = \varepsilon z_0$. Unwinding the steps gives the cycle equation
$$z_0(\varepsilon\pi^V - \alpha^n) = \sum_{j=0}^{n-1} d_j\,\alpha^{n-1-j}\,\pi^{v_0 + \cdots + v_{j-1}}.$$
The right side has size about $|\pi|^V$ times a bounded factor, so $|z_0|$ is of order $1/\text{rel}$ with $\text{rel} = |1 - \alpha^n/(\varepsilon\pi^V)|$, and cycles of large elements need $\alpha^n$ close to $\varepsilon\pi^V$. In $\mathbb{Z}$ this is the condition that $V/n$ approximate $\log_2 |m|$. The cycle of $3x+1$ through $-17$ has $n = 7$ odd steps and $V = 11$, and $11/7$ is an intermediate fraction of $\log_2 3$; since $2^{11} < 3^7$, the equation forces $x_0 < 0$ (computed; the cycles through $-1$ and $-5$ also sit at such fractions). For the maps of Section 3.3 the cycles concentrate at near-coincidences of $m^n$ with $2^V$, in numbers close to the heuristic count
$$E(n, V) = \frac{2^{(k-1)n}\binom{V - kn + n - 1}{n-1}}{2n\,|2^V - m^n|}.$$
At modulus $128$, $m = 181$ has $181^2 = 0.999786 \cdot 2^{15}$, $293$ cycles of two odd steps against $E = 292.6$, and $294$ cycles in all; at modulus $512$, $m = 645$ has $85$ cycles of three odd steps against $E = 84.5$ (counts computed to $10^6$, $E$ heuristic). The count comes from the $2^{(k-1)n}$ residue classes of odd $2$-adic integers with given valuations, each of which holds one solution of $S^n(x) = \varepsilon x$, together with the guess that this solution is an integer with probability $1/|2^V - m^n|$.

### 3.2 What stops mattering

On $\mathbb{N}$ a Collatz map has to reach $1$. On $\mathbb{Z}$ it has to reach a trivial cycle, a cycle through $1$ or $-1$, and which of them it reaches stops mattering. The digits and the multiplier may be negative, since nothing has to stay positive, and the norm $N(m) = |m|$ places a negative multiplier in the same window as its absolute value. The number of trivial cycles is bounded by the number of units.

*Proposition 3.1.* A map on any of the three rings has at most $|U|$ trivial cycles.

*Proof.* $S$ is a function, so two cycles that meet are equal (if $x$ is periodic with period $p$ and $y = S^j(x)$, then $S^{j(p-1)}(y) = x$). So distinct trivial cycles are disjoint, and each contains a unit. $\square$

In $\mathbb{Z}$ there are at most two. For $7x \pm 1$ they are the fixed points $1$ and $-1$; for $-7x \pm 1$, the loop $1 \leftrightarrow -1$ (computed).

### 3.3 What becomes important: the unit $-1$

On $\mathbb{N}$ there is nothing to ask about $-1$. On $\mathbb{Z}$ one can ask whether a map commutes with it, that is, whether $T(-x) = -T(x)$, and $3x+1$ does not.

*Proposition 3.2.* No map on $\mathbb{Z}$ at modulus $2$ commutes with $-1$.

*Proof.* There is one odd class modulo $2$, so $d(-x) = d(x)$, while commuting requires $d(-x) = -d(x)$; so $d(x) = 0$, which is even, against Lemma 2.2. $\square$

The window forces multiplier $3$ to modulus $2$, since $2 < 3 < 4$, so no map with multiplier $\pm 3$ commutes with $-1$. We did not find this one-line observation stated in any source we read. The four maps $\pm 3x \pm 1$ also fail to be Collatz maps on $\mathbb{Z}$, and for multiplier $3$ no other digit helps. This follows from a lemma on fixed points.

*Lemma 3.3 (forced fixed points).* Let $(\alpha, k, d)$ be a map, $v \ge k$, $\varepsilon$ a unit and $z$ odd, and suppose $\delta = (\varepsilon\pi^v - \alpha)z$ lies in $D$. Then $S(z) = \varepsilon z$. If moreover $z$ is not a unit, and $\varepsilon = 1$ or the map commutes with the units, then the cycle of $z$ contains no unit and the map is not a Collatz map.

*Proof.* Since $v \ge k$, $\delta \equiv -\alpha z \pmod{\pi^k}$, so $\delta$ is the digit of $z$ by Lemma 2.2, and $\alpha z + \delta = \pi^v(\varepsilon z)$ with $\varepsilon z$ odd. If $\varepsilon = 1$, $z$ is fixed. If the map commutes with the units, $S^n(z) = \varepsilon^n z$. In both cases every element of the cycle has norm $N(z) > 1$. $\square$

*Corollary 3.4.* Let $k \ge 1$, $v \ge k$ and $\varepsilon$ a unit with $\eta = \varepsilon\pi^v - \alpha$ a unit, where $\varepsilon = 1$ unless the map commutes with the units. If the map is a Collatz map, every digit is a unit.

*Proof.* For a digit $\delta$, put $z = \eta^{-1}\delta$, which is odd and satisfies $(\varepsilon\pi^v - \alpha)z = \delta$. If $\delta$ were not a unit, neither would $z$ be, and Lemma 3.3 would apply. $\square$

Both are proved for any of the three rings, and for $\mathbb{Z}[i]$ in Lean (`Expr.fixed_point`, `Expr.fixed_point_not_rule`, `Expr.digits_units`). For $m = 3$ at modulus $2$, $2^2 - 3 = 1$, so a Collatz map with multiplier $3$ has digit $\pm 1$. Then $3x+1$ has the cycles through $-5$ and $-17$, and $3x - 1$ the cycles through $5$ and $17$, so no map with multiplier $3$ is a Collatz map on $\mathbb{Z}$ (proved). For $m = -3$ the numbers $2^v + 3$ are never units and the corollary says nothing; $-3x \pm 1$ have a cycle of $12$ odd steps through $\pm 13$ (computed to $10^6$), and other digits for $-3$ were not tried.

So at modulus $2$ no map commutes with $-1$, and the maps $3x \pm 1$ fail. We note the coincidence without claiming a cause. The failures come from exhibited cycles, Section 4 shows Collatz maps on $\mathbb{Z}[i]$ that do not commute with the units, and below we find such maps on $\mathbb{Z}$ as well.

From modulus $4$ on, commuting with $-1$ comes free with the simplest digits.

*Lemma 3.5.* For $k \ge 2$, each odd class modulo $2^k$ has a unique element of least absolute value, and these elements form the set $D_k = \{d \text{ odd} : |d| < 2^{k-1}\}$, which is closed under $x \mapsto -x$. With the digit set $D_k$ and $2^k < m < 2^{k+1}$, $S(1) = 1$.

*Proof.* The interval $(-2^{k-1}, 2^{k-1})$ has length $2^k$ and even endpoints, so it holds exactly one element of each odd class, and every other element of the class has absolute value above $2^{k-1}$. For $S(1)$, the digit $d$ of $1$ satisfies $2^k \mid m + d$ and $2^k - 2^{k-1} < m + d < 2^{k+1} + 2^{k-1}$, so $m + d$ is $2^k$ or $2^{k+1}$. $\square$

At modulus $4$, $D_2 = \{\pm 1\}$, and the two maps in the window are $7x \pm 1$, Barina's map, and $5x \pm 1$, with digit $-1$ for $x \equiv 1$ and $+1$ for $x \equiv 3 \pmod 4$. Every known natural Collatz map on $\mathbb{Z}$, Barina's included, commutes with $-1$. For $7x \pm 1$ Barina's verification to $10^{15}$ [Barina 2018] supersedes our own check to $10^7$, which we keep only as a test of our code. The map $5x \pm 1$ has, besides the fixed point $1$, the cycle
$$7 \to 36 \to 18 \to 9 \to 44 \to 22 \to 11 \to 56 \to 28 \to 14 \to 7$$
and its negative (computed). We did not find this map or its cycle in the literature or the OEIS, as far as we searched: OEIS searches for the values of the map ($4, 1, 16, 2, 24, 3, 36, 4, 44, 5, 56$), of its accelerated form ($1, 1, 3, 9, 11, 7, 1, 19, 21$) and for the cycle, without signs, found nothing. Up to sign, the stretch $36, 18, 9, 44, 22, 11, 56, 28, 14, 7$ does occur in del Arte's orbit of $5$ under $-5x+1$ [OEIS A305057], whose terms there are $36, 18, 9, -44, -22, -11, 56, 28, 14, 7$; but $-5x+1$ sends $7$ to $-34$, so the orbit does not close.

The fixed points of the maps with digits $D_k$ follow from Lemma 3.3.

*Proposition 3.6.* Let $k \ge 2$, $m$ odd with $2^k < m < 2^{k+1}$, and $\delta = \min(m - 2^k, 2^{k+1} - m)$.

1. With digits $D_k$, an odd $x > 0$ is a fixed point exactly when $x < 2^{k-1}/\delta$, and no odd $x$ has $S(x) = -x$. There are $\lfloor (2^{k-1}/\delta + 1)/2 \rfloor$ fixed points up to sign, at least two exactly when $m < \frac{7}{6}2^k$ or $m > \frac{11}{6}2^k$.
2. With the positive digit set, $1$ and $-1$ are fixed for every $m$.

*Proof.* (1) By Lemma 3.3 with $\pi = 2$, $x$ is fixed with sign $\varepsilon$ and exponent $v \ge k$ exactly when $|(\varepsilon 2^v - m)x| < 2^{k-1}$. For $\varepsilon = -1$, $|{-2^v} - m| > 2^{k-1}$; for $v \ge k+2$, $2^v - m > 2^{k+1}$. The values $v = k$ and $v = k+1$ give $m - 2^k$ and $2^{k+1} - m$, and exactly one of them is below $2^{k-1}$ since $m \ne 3 \cdot 2^{k-1}$. So the condition is $\delta x < 2^{k-1}$, and since $\delta$ is odd, $2^{k-1}/\delta$ is not an odd integer, which gives the count. (2) The digits lie in $(0, 2^k)$, and $(2^{k+1} - m) \cdot 1$ and $(m - 2^k) \cdot 1$ are positive digits less than $2^k$. So $S(1) = 1$ with $v = k+1$, and $S(-1) = -1$ with $v = k$. $\square$

So in every window from modulus $8$ on, the outer sixths, $m/2^k < 7/6$ and $m/2^k > 11/6$, hold no Collatz map with the smallest digits, and no map with the positive digit set is a Collatz map on $\mathbb{Z}$. At $k = 2$ both $5$ and $7$ have $\delta = 1$, so Proposition 3.6 gives only the fixed point $1$; the second cycle of $5x \pm 1$ is the $3$-cycle through $7$.

The integer census covers every map with digits $D_k$ at moduli $4$ to $1024$, $1022$ maps (Table 1). Every odd start with $|x| \le 10^6$ was followed to its cycle in exact arithmetic. For each map with a single cycle up to sign, a drop-below-start check showed that every odd $x$ with $3 \le x \le 10^7$ reaches a smaller $|y|$, so with $S(1) = 1$ and $S(-x) = -S(x)$, every odd $|x| \le 10^7$ reaches $\pm 1$; these maps are Collatz maps to that bound. Every other map has at least two cycles, exhibited and checked independently in exact arithmetic for $765$ maps and given by Proposition 3.6 for the rest (computer-assisted).

*Table 1. Maps on $\mathbb{Z}$ with digits $D_k$, moduli $4$ to $1024$.*

| modulus | maps | Collatz maps to $10^7$ | first such $m$ | excluded by Proposition 3.6 |
|---|---|---|---|---|
| 4 | 2 | 1 | 7 | 0 |
| 8 | 4 | 0 | none | 2 |
| 16 | 8 | 2 | 21 | 2 |
| 32 | 16 | 4 | 41 | 6 |
| 64 | 32 | 5 | 77 | 10 |
| 128 | 64 | 15 | 153 | 22 |
| 256 | 128 | 30 | 307 | 42 |
| 512 | 256 | 67 | 611 | 86 |
| 1024 | 512 | 127 | 1223 | 170 |

The $251$ Collatz maps have $m/2^k$ in $[1.193, 1.832]$ and drift between $-0.745$ and $-0.127$ bits, nearly filling the range $(\log_2\frac{7}{12}, \log_2\frac{11}{12}) = (-0.778, -0.126)$ that Proposition 3.6 allows. Their share settles near a quarter from modulus $128$ on. Inside the window they are missing near the resonances: the tenth of the window around $m/2^k = \sqrt 2$ holds none of its $102$ maps, because there $m^2 \approx 2^{2k+1}$ and the maps have many cycles of two odd steps (at modulus $1024$ every $m$ from $1391$ to $1469$ has one, $53$ for $m = 1449$, where $1449^2/2^{21} = 1.0012$). No orbit appeared to diverge at any modulus up to $1024$; orbits reached $2^{1322}$ and returned, and integers have no invariant lines like those of Section 4.3. The scores run from $1/4 - 1/7 = 0.107$ for $7x \pm 1$ down through $0.0225$ for $25x \pm 1$ and $0.0149$ for $21x \pm 1$ at modulus $16$.

Maps that do not commute with $-1$ can be Collatz maps on $\mathbb{Z}$, but by Lemma 3.5 they need a digit that is not the smallest in its class, and at modulus $4$ also a negative multiplier. For $m > 0$ at modulus $4$, Corollary 3.4 applies to both multipliers ($2^2 - 5 = -1$ and $2^3 - 7 = 1$) and forces the digits $\pm 1$. For $m < 0$, $2^v - m$ is never a unit and the corollary says nothing. Of $80$ random non-commuting tables at modulus $4$ with $|d| \le 15$ and $m = \pm 5, \pm 7$, six are Collatz maps to $10^6$, all with $m < 0$, among them $m = -5$ with digits $13$ and $3$ for the classes $1$ and $3$ modulo $4$, and $m = -7$ with $-3$ and $11$ (computed to $10^6$). A search for maps with the unit loop $1 \leftrightarrow -1$ found $5$ Collatz maps at modulus $4$ ($-7x \pm 1$ and four with $m = -5$, for instance digits $1$ and $11$), $106$ at modulus $8$ and $201$ in a sample of $3000$ at modulus $16$, and at modulus $16$ positive multipliers occur, for instance $m = 21$ with digits $17, 3, 37, 23, -7, -37, 13, -1$ for the classes $1, 3, \ldots, 15$ (computed to $10^6$). The search, the random tables and the proofs are in `symmetry-test.md`.

## 4. From $\mathbb{Z}$ to $\mathbb{Z}[i]$

### 4.1 What stops mattering

The modulus of $3x+1$ disappears. At modulus $1+i$ the window is $2 < N(\alpha) < 4$, so $N(\alpha) = 3$, and $3$ is not a sum of two squares, since squares are $0$ or $1$ modulo $4$ (proved; in Lean, `prop3_int` and `prop3`, without the step from the window to $N(\alpha) = 3$). At modulus $1$ the window $(1, 2)$ holds no integer. A map with one odd class and one digit, the shape of $3x+1$, has no counterpart inside the window, for the same reason that $3$ stays prime in $\mathbb{Z}[i]$. The REU map of Alvarado and his students sits one modulus too low: it uses modulus $1+i$ with $N(2+i) = 5$ above $(2, 4)$, so its drift is $\log_2 5 - 2 = +0.32$ bits per odd step, which fits the $5.6\%$ of starts that reached a unit.

Nor does $3x+1$ survive as a restriction. The multiplier $3$ has norm $9$ and falls at modulus $(1+i)^3$. If a map with $\alpha = 3$ takes the odd step $n \mapsto 3n+1$ at every positive odd integer, then $(1+i)^k$ divides $10 = -i(1+i)^2(2+i)(2-i)$, so $k \le 2$ and $9 > 2^{k+1}$: the map is above the window (proved; the step $k \le 2$ in Lean, `prop5_2`, `prop5_2'`). So no map in the family both agrees with $3x+1$ on the positive integers and contracts on average in the plane.

### 4.2 What persists

The window persists with $q = 2$. Odd $\alpha$ has $N(\alpha) \equiv 1 \pmod 4$, and the windows hold these multipliers up to units:

| modulus | window | odd norms | multipliers up to units |
|---|---|---|---|
| $1+i$ | $(2, 4)$ | none | none |
| $(1+i)^2$ | $(4, 8)$ | 5 | $2 \pm i$ |
| $(1+i)^3$ | $(8, 16)$ | 9, 13 | $3$, $3 \pm 2i$ |
| $(1+i)^4$ | $(16, 32)$ | 17, 25, 29 | $4 \pm i$, $5$, $4 \pm 3i$, $5 \pm 2i$ |
| $(1+i)^5$ | $(32, 64)$ | 37 to 61 (6 norms) | 11 |
| $(1+i)^6$ | $(64, 128)$ | 65 to 125 (12 norms) | 28 |
| $(1+i)^7$ | $(128, 256)$ | 137 to 245 (18 norms) | 46 |

Proposition 3.1 persists with $|U| = 4$ (in Lean, `trivial_cycles_le_four`), and so do Lemma 3.3 and Corollary 3.4. The cycle equation of Section 3.1 persists, now with a second condition: for non-real $\alpha$, $\alpha^n$ must be close to $\varepsilon(1+i)^V$ in angle as well as in modulus. Where both hold, cycles crowd. At modulus $(1+i)^7$, $(11+10i)^{33}$ is within $\text{rel} = 6.17 \cdot 10^{-4}$ of a unit times $(1+i)^{257}$, and the map with $\alpha = 11 + 10i$ and the smallest digits has $728$ cycles of $33$ odd steps up to units, against a heuristic count of $764$ (computed; [Paper 2] gives the complete lists up to stated lengths).

### 4.3 What becomes important

*Collatz maps at the smallest modulus.* The first modulus with multipliers in the window is $(1+i)^2 = 2i$, where classes are classes modulo $2$ and the odd classes are those of $1$ ($a$ odd, $b$ even) and of $i$. The multipliers are the eight associates of $2 \pm i$. The class of $1$ has the two smallest elements $\pm 1$ and the class of $i$ has $\pm i$, so there are four tables with the smallest digits, and the conjugacy $z \mapsto uz$, which carries $(\alpha, D)$ to $(\alpha, u^{-1}D)$, makes them one orbit. So there are eight maps up to conjugacy, one for each multiplier, with $D = \{1, i\}$. Five are Collatz maps and three are not:

| $\alpha$ | trivial cycles | other cycles | Collatz map |
|---|---|---|---|
| $2 - i$ | $1 \to -i \to -1 \to 1$ | none | yes |
| $-1 - 2i$ | $1 \to -1 \to -i \to 1$ | none | yes |
| $-2 - i$ | $1 \to i \to -1 \to -i \to 1$ | none | yes |
| $1 + 2i$ | fixed points $-1$, $-i$ | none | yes |
| $-2 + i$ | fixed points $1$, $-i$ | none | yes |
| $2 + i$ | $1 \to -i \to -1 \to i \to 1$ | through $5-2i, 1-6i, -5-4i, -6+3i, 7i$ | no |
| $1 - 2i$ | $\{1, -1\}$, $\{i, -i\}$ | two cycles of two odd steps | no |
| $-1 + 2i$ | four fixed units | five | no |

For the five, every odd $z$ with $1 < N(z) \le 10^7$ falls below its norm, over all four quadrants ($15{,}708{,}048$ starts each, longest stopping time $260$ steps of $T$), and by a soundness theorem that needs no condition on units (in Lean, `drop_below_start_sound_all`) every orbit with $N(z) \le 10^7$ reaches a unit (computed to $10^7$). In full, the first map is
$$T(z) = \begin{cases} z/(1+i) & a + b \text{ even}, \\ (2-i)z + i & a \text{ odd}, b \text{ even}, \\ (2-i)z + 1 & a \text{ even}, b \text{ odd}. \end{cases}$$
Its steps $(2-i) + i = 2 = (1+i)^2(-i)$, $(2-i)(-i) + 1 = -2i = (1+i)^2(-1)$ and $-(2-i) + i = -2+2i = (1+i)^3$ give the unit cycle $1 \to -i \to -1 \to 1$, and $(2-i)i + 1 = 2 + 2i = (1+i)^3(-i)$ sends $i$ into it, so $i$ is not periodic. For this map, the table, the cycle, the fact that $i$ is not periodic, and the reduction of the $10^7$ claim to the run are proved in Lean (`digit2_correct`, `E2_cycle`, `E2_I_not_periodic`, `E2_sound`). Its drift is $\log_2(5/8) = -0.68$ bits per odd step and its score $1/4 - 1/5 = 0.05$, equal to that of $5x \pm 1$.

The REU multiplier $2 + i$ is in the window at this modulus, and with these digits it fails, while its mirror $2 - i$ is a Collatz map. Collatz maps without any condition on the units are plentiful here: with each digit chosen among the odd elements of norm at most $5$, all $288$ maps with $\alpha$ an associate of $2 \pm i$ reached cycles for $N(z) \le 10^5$, and $68$ had a single nonzero cycle (computed to $10^5$). No map at this modulus commutes with the units, since $-1 \equiv 1 \pmod{(1+i)^2}$ (Section 6).

*Modulus $(1+i)^3$.* The four odd classes modulo $(1+i)^3$ each hold exactly one unit, and the twelve multipliers are the associates of $3$ and $3 \pm 2i$. The only table that commutes with the units has $D = U$, by Corollary 3.4, and none of those maps is a Collatz map:

*Proposition 4.1.* A map at modulus $(1+i)^3$ in the window that commutes with the units is not a Collatz map, whatever its digit table.

*Proof.* Rotating by a unit (Section 6) reduces to $\alpha \in \{3, 3+2i, 3-2i\}$, and rotation preserves being a Collatz map for maps that commute with the units. With $\pi = 1+i$, $\pi^3 = -2+2i$ and $\pi^4 = -4$: $-\pi^4 - 3 = 1$, $-i\pi^3 - (3+2i) = -1$ and $-\pi^3 - (3-2i) = -1$, so Corollary 3.4 forces $D = U$. Then $\alpha = 3$ sends $-2-i \to 1+2i \to 1-2i = i(-2-i)$, $\alpha = 3+2i$ sends $-3 \to 1+4i \to 5-2i \to -3+2i \to 3$, and $\alpha = 3-2i$ sends $-3 \to 4+i \to -5-2i \to 2-3i \to 3i$, with digits that are units. The unit multiples of these points form a set without units that $S$ maps into itself. $\square$

This is the case $k = 3$ of the bound that [Paper 2] proves, and it is in Lean (`Expr.no_symmetric_rule_below_29`). Without the condition on the units the picture changes. With each digit among the six smallest elements of its class, $1204$ tables pass a drop-below-start check to $N(z) \le 10^6$ over all quadrants, $305$ maps up to the conjugacy $z \mapsto uz$, and they cover $8$ of the $12$ multipliers: $\pm 3i$ and all associates of $3 \pm 2i$ except $-3+2i$ and $-2+3i$ (computed to $10^6$; one representative per multiplier to $10^7$). For $\alpha = 3i$ the table that gives the classes of $1, i, -1, -i$ the digits $2-i, -1, -i, 1$ has the single trivial cycle $1 \to -i \to -1 \to i \to 1$, which can be checked by hand: $3i + (2-i) = 2+2i = (1+i)^3(-i)$, $3i(-i) + 1 = 4 = (1+i)^4(-1)$, $3i(-1) - i = -4i = (1+i)^4 i$, and $3i \cdot i - 1 = -4 = (1+i)^4$. For six of the eight multipliers some Collatz table differs from $D = U$ in one class.

*Proposition 4.2.* For $\alpha \in \{-3, -3+2i, -2+3i\}$ there is no Collatz map at modulus $(1+i)^3$, whatever the digit table.

*Proof.* $\pi^4 + 3 = -1$, $\pi^3 - (-3+2i) = 1$ and $\pi^3 - (-2+3i) = -i$ are units, so Corollary 3.4 with $\varepsilon = 1$, which needs no condition on the units, forces every digit to be a unit. The units lie in distinct classes, so $D = U$, which commutes with the units, and Proposition 4.1 applies. $\square$

The multiplier $3$ is open. Lemma 3.3 removes no digit of norm at most $100$ for it, no table with the six smallest digits passes, $6868$ of the $1{,}500{,}625$ tables with digits of norm at most $100$ pass at $N(z) \le 10^4$, and of a random sample of $30$ of those, $11$ still pass at $10^6$ (computed). An impossibility proof would need an argument that every table has some long cycle without a unit. The associates $3$ and $3i$ behave differently because rotating a map that does not commute with the units relates only one step of the two maps: $S_{u\alpha}(z) = S_\alpha(uz)$ (Section 6).

*The units on unequal footing.* Without a condition on the units, the trivial cycles of a map need not treat the units alike. The map $2 - i$ above has a unit cycle of length $3$, which is not the order of any unit, and the unit $i$ lies off it. At modulus $(1+i)^3$, Collatz maps have a unit cycle through all four units ($3i$ above), a lone $2$-cycle $\{i, -i\}$ ($3 - 2i$ with digits $1-2i, -i, 1, i$), or mixed trivial cycles through units and non-units ($-3i$ with digits $2-3i, 1, i, -1$, whose one trivial cycle has ten odd elements). At modulus $(1+i)^4$ the map with $\alpha = -2-5i$ and digits $\pm 1, \pm i, 1-2i, -2+i, 2-i, -1-2i$ has three fixed units, $1$, $-1$ and $i$, and three does not divide four (computed to $10^5$). Section 6 shows that under the condition on the units none of this can happen.

*Divergence on lines.* Integers in the window showed no divergent orbit. In $\mathbb{Z}[i]$, among the maps with the smallest digits that commute with the units, orbits that appear to diverge occur only for the real multipliers $5$ at modulus $(1+i)^4$ and $9$, $11$ at $(1+i)^6$, and every escaping orbit ends on one of the eight lines
$$L: \quad x = \pm 1, \quad y = \pm 1, \quad x - y = \pm 1, \quad x + y = \pm 1.$$
For every real multiplier tested from $3$ to $15$, $S$ maps $L$ into itself, checked over a full residue period (computer-assisted). On $L$ the map is one-dimensional, and a two-state model of its valuations gives a drift along $L$ of $+0.122$, $+0.059$ and $+0.187$ bits of $|z|$ per odd step for $5$, $9$ and $11$, and negative values for $3$, $7$, $13$ and $15$ (heuristic). The sign agrees with the observed behaviour in every case. In $\mathbb{Z}$ the line is the whole ring, and every map in the window has negative drift. [Paper 2] treats the lines in detail.

## 5. From $\mathbb{Z}[i]$ to the Eisenstein integers

Write $z = a + b\omega$ with $\omega^2 = -1 - \omega$, so $N(z) = a^2 - ab + b^2$. The prime $2$ is inert, so $\pi = 2$, $q = 4$, and $z$ is odd when $a$ and $b$ are not both even. There are six units, $\pm 1, \pm\omega, \pm\omega^2$, and each class of associates has exactly one element with $0 \le b < a$.

### 5.1 What persists

The window persists in the general form of Definition 2.6, $4^k < N(\alpha) < 4^{k+1/3}$. At modulus $2$ it is $(4, 6.35)$ and holds no norm, so here too the first modulus has no multiplier. At modulus $4$ it is $(16, 25.40)$, with the odd norms $19$, $21$ and $25$ and five multipliers up to units: $5 + 2\omega$ and its conjugate $5 + 3\omega$ (norm $19$), $5 + \omega$ and $5 + 4\omega$ (norm $21$), and $5$. The naive window $(4^k, 4^{k+1})$ would admit $23$ multipliers up to units at modulus $4$, and all but these five expand on average. Proposition 3.1 persists with at most six trivial cycles, and Lemma 3.3 again accounts for the fixed points.

### 5.2 What becomes important

The first modulus with multipliers, $4$, is also the first at which maps can commute with the units (Section 6). Its $12$ odd classes form two orbits of six under the units, each class has a unique element of least norm, and these elements, the units and the six associates of $1 - \omega$ (norm $3$), make up the only table with the smallest digits, $D = U \cup (1-\omega)U$. So, as in $\mathbb{Z}$ from modulus $4$ and unlike $\mathbb{Z}[i]$ at $(1+i)^2$, the simplest maps commute with the units. Up to units and conjugation there are three maps:

| $\alpha$ | $N(\alpha)$ | drift (bits) | cycles up to units, $N(z) \le 10^5$ |
|---|---|---|---|
| $5 + 2\omega$ | 19 | $-0.419$ | 1, the fixed class of $1$ |
| $5 + \omega$ | 21 | $-0.274$ | 3: $1$, $2 + \omega$ (both fixed), and $7 + 3\omega$ (11 odd steps) |
| $5$ | 25 | $-0.023$ | 84 found, 9104 starts unresolved |

For $5 + 2\omega$ and $5 + 3\omega$, every odd $z$ with $1 < N(z) \le 10^6$ falls below its norm, in exact arithmetic, so every odd $z$ with $N(z) \le 10^6$ reaches a unit (computed to $10^6$). Read literally, with cycles as sets and all six associates of each multiplier, $12$ of the $30$ maps are Collatz maps at $N(z) \le 10^5$, namely the associates of $5 + 2\omega$ and $5 + 3\omega$ (computed to $10^5$). So $\mathbb{Z}[\omega]$ has a Collatz map that commutes with the units at its first modulus, with the smallest multiplier there; its score is $1/16 - 1/19 = 0.0099$.

Lemma 3.3 with $\pi^2 = 4$ explains the fixed classes. For $5 + \omega$, $4 - \alpha = \omega^2$ is a unit, so by Corollary 3.4 a Collatz map would need unit digits, and the non-unit digits give fixed points such as $2 + \omega$. For $\alpha = 5$, $4 - 5 = -1$. For $5 + 2\omega$, $4 - \alpha = -1 - 2\omega$ has norm $3$ and lies in $D$, so the class of $1$ is fixed and no other $z$ passes the lemma.

Six units give more ways for a trivial cycle to sit among them. Among the twelve Collatz maps above, the literal trivial cycles are one cycle through all six units (for $-2-5\omega$, $2-3\omega$, $3+5\omega$, $5+3\omega$), two cycles of three units each ($-5-3\omega$, $-3-5\omega$, $-2+3\omega$, $2+5\omega$), three cycles of two units ($-5-2\omega$, $-3+2\omega$), or six fixed units ($3-2\omega$, $5+2\omega$). The counts $1, 2, 3, 6$ are the divisors of $6$ that Theorem 6.2 allows.

The real multiplier recurs as the divergent case. For $\alpha = 5$, $59$ of $139$ random odd starts with $|a|, |b| \le 300$ passed norm $2^{400}$, and each ended on a line at distance one from a line of units, $b = 1$ ($33$ starts) or $a - b = 1$ ($26$), in the canonical sector (computed).

At modulus $8$ the window $(64, 101.6)$ gives $16$ multipliers up to units, with norms $67$ to $97$. The $48$ odd classes form eight orbits, one of which has two elements of least norm, $2 - 3\omega$ and $2 + 5\omega$, so there are two tables with the smallest digits that commute with the units, conjugate to each other, and $32$ maps. None has a single cycle at $N(z) \le 10^5$. Most have two or three cycles, and $\alpha = 9$ has $42$ with $5829$ starts unresolved; $45$ of $139$ random starts for $\alpha = 9$ passed norm $2^{400}$, all on the same two lines (computed to $10^5$). The tied orbit also gives $64$ tables with the smallest digits in all, and in a sample of two random non-commuting tables for each of the $96$ associates, $18$ of $192$ maps are Collatz maps at $N(z) \le 10^5$, all of norm $91$ (computed to $10^5$).

The REU slides mention an Eisenstein analogue that divides by the ramified prime $1 - \omega$, with $q = 3$, without computations [Alvarado 2025]. Our maps divide by the inert prime $2$, and the version with $1-\omega$ is untested.

## 6. What emerges as the units grow

### 6.1 When maps can commute with the units

In $\mathbb{N}$ there is nothing to commute with. In $\mathbb{Z}$ no map commutes with $-1$ at modulus $2$ (Proposition 3.2), and from modulus $4$ the smallest digits commute automatically (Lemma 3.5). In $\mathbb{Z}[i]$ no map commutes with the units at moduli up to $(1+i)^2$: since $-1 - 1 = -2 = i(1+i)^2$, $z$ and $-z$ share a class, so $d(-z) = d(z) = -d(z)$ and $d(z) = 0$ (in Lean, `prop1`). In $\mathbb{Z}[\omega]$ maps commuting with the units first exist at modulus $4$. The general statement is a theorem about Dedekind domains.

*Theorem 6.1.* Let $R$ be a Dedekind domain, $P$ a nonzero prime ideal with finite residue field, and $U$ a finite group of units, and call $z$ odd if $z \notin P$. Write $\kappa = \max_{u \in U,\, u \ne 1} v_P(u - 1)$. For $k \ge 0$ the following are equivalent:

1. for some odd $\alpha$ there is a map at modulus $P^k$ that commutes with $U$;
2. for every odd $\alpha$ there is such a map;
3. no $u \in U$ with $u \ne 1$ satisfies $u \equiv 1 \pmod{P^k}$, that is, $k > \kappa$;
4. $U$ acts freely on the odd classes modulo $P^k$.

*Proof.* (3) and (4): for odd $z$, $uz \equiv z \pmod{P^k}$ means $(u-1)z \in P^k$, and since $z \notin P$ this holds exactly when $u - 1 \in P^k$. (1) implies (3): if $u \ne 1$ and $u \equiv 1 \pmod{P^k}$, then $uz$ and $z$ share a class, so $d(uz) = d(z)$, while commuting gives $d(uz) = u\,d(z)$; hence $(1-u)d(z) = 0$ and $d(z) = 0$, which is not odd. (4) implies (2): if $U$ is trivial there is nothing to prove. Otherwise $k \ge 1$, the odd classes fall into orbits of exactly $|U|$ classes, and we choose one class $c$ in each orbit, an element $r \in c$ and an element $\delta_c \equiv -\alpha r \pmod{P^k}$, which is odd. Each odd class is $uc$ for exactly one $c$ and one $u$, and $d(z) = u\delta_c$ on the class $uc$ is a digit table with $d(wz) = w\,d(z)$ for $w \in U$. (2) implies (1) trivially. $\square$

The proof shows that, up to the choice inside each class, a table that commutes with the units is a choice of one digit for one class in each orbit. With $-1 \in U$ and $P$ above $2$, $v_P(-2) = e$, the ramification index, so $\kappa \ge e$.

| ring | $U$ | $\kappa$ | first modulus with maps commuting with $U$ |
|---|---|---|---|
| $\mathbb{Z}$ | $\pm 1$ | 1 | $4$ |
| $\mathbb{Z}[i]$ | order 4 | 2 | $(1+i)^3$ |
| $\mathbb{Z}[\omega]$ | order 6 | 1 | $4$ |
| other imaginary quadratic fields | $\pm 1$ | $e$ | $P^2$ if $2$ is unramified, $P^3$ if it ramifies |

In $\mathbb{Z}[\omega]$, $\omega - 1$ and $\omega^2 - 1$ are odd (norm $3$), while $-\omega - 1 = \omega^2$ and $-\omega^2 - 1 = \omega$ are units, so $\kappa = v_2(-2) = 1$. If the unit group is infinite, as in a real quadratic field, then for every $k$ the kernel of $U \to (R/P^k)^\times$ has finite index in $U$ and so contains some $u \ne 1$: no modulus admits a map that commutes with all units. Commuting with the torsion units $\pm 1$ alone is possible from $k > e$. All of this is proved on paper (Theorem 6.1 and the thresholds are not in Lean).

Above the threshold, whether the smallest digits commute with the units is a property of the lattice $\pi^k R$. If every odd class has a unique element of least norm, the table of those elements commutes with the units, because multiplication by a unit carries a class to a class and preserves norms. Ties lie on the boundary of the Voronoi cell of the lattice. In $\mathbb{Z}$ the only tie is $\pm 1$ at modulus $2$. In $\mathbb{Z}[i]$ there are no ties at odd $k \ge 3$, and at even $k = 2j$ there are $2^j$ tied classes and $2^{2^j}$ tables with the smallest digits, of which $2^{2^{j-2}}$ commute with the units for $j \ge 2$ ($4$ and $0$ at $(1+i)^2$, $16$ and $2$ at $(1+i)^4$, $256$ and $4$ at $(1+i)^6$; proved, and counted by computer). In $\mathbb{Z}[\omega]$ there is no tie at modulus $4$ and one tied orbit at modulus $8$.

### 6.2 What commuting with the units buys

The maps of Section 4.3 showed the units on unequal footing. Under the condition on the units this cannot happen. In the three rings $U$ is cyclic, of order $2$, $4$ or $6$.

*Theorem 6.2.* Let $f$ be a function on $R$ with $f(uz) = uf(z)$ for all $u \in U$, for instance $S$ of a map that commutes with the units.

1. If $1$ is not periodic, no unit is, and there are no trivial cycles. If $1$ is periodic with cycle $C_1$, the trivial cycles are the sets $uC_1$, the stabilizer of $C_1$ is the subgroup $U \cap C_1$, and the number of trivial cycles is $|U|/|U \cap C_1|$, a divisor of $|U|$.
2. If $w = f(1)$ is a unit, then $f^n(u) = uw^n$, every unit lies on a unit cycle, every unit cycle has length equal to the order of $w$, and there are $|U|/\text{ord}(w)$ of them. If $w$ is not a unit, there is no unit cycle.

*Proof.* (1) $f^n(u) = uf^n(1)$, so $u$ is periodic exactly when $1$ is, with cycle $uC_1$, and a trivial cycle through $u$ is $uC_1$ because cycles that meet are equal. If $uC_1 = C_1$ then $u \in C_1$; if $u \in C_1$, then $uC_1$ and $C_1$ are cycles through $u$, so they are equal. $U \cap C_1$ contains $1$ and is closed under products ($u, u' \in C_1$ gives $uu' \in uC_1 = C_1$), so it is a subgroup of the finite group $U$, and the cycles correspond to its cosets. (2) $f(u) = uf(1) = uw$, and induction gives $f^n(u) = uw^n$, which equals $u$ exactly when $w^n = 1$. If $w$ is not a unit, $N(uw) = N(w) \ne 1$. $\square$

For $\mathbb{Z}[i]$ both parts are in Lean (`trivial_cycle_count`, `unit_cycles`, `no_unit_cycle`). Without the condition, part (1) fails for the map $-2-5i$ of Section 4.3 with three trivial cycles, and part (2) fails for $2 - i$, whose unit cycle has length $3$ with $i$ off it (in Lean, `E2_cycle`, `E2_I_not_periodic`). In $\mathbb{Z}$ the counts are $2$ for $m > 0$ with digits $D_k$, since $S(1) = 1$, and $1$ for $-m$, whose loop is $1 \leftrightarrow -1$.

The condition also ties a map to its rotations and mirror image. For a unit $u$, the rotated map of $(\alpha, k, d)$ is $(u\alpha, k, d^u)$ with $d^u(z) = d(uz)$; it has the same digit set. On $\mathbb{Z}[i]$ the mirrored map is $(\bar\alpha, k, \bar d)$ with $\bar d(z) = \overline{d(\bar z)}$.

*Proposition 6.3 (one step, no condition on the units).* $S_{u\alpha}(z) = S_\alpha(uz)$ for every map. On $\mathbb{Z}[i]$, $S_{\bar\alpha}(z) = (-i)^{v}\,\overline{S_\alpha(\bar z)}$ with $v$ the exponent of $\pi$ in $\alpha\bar z + d(\bar z)$, so $N(S_{\bar\alpha}(z)) = N(S_\alpha(\bar z))$.

*Proof.* $u\alpha z + d(uz) = \alpha(uz) + d(uz)$. For the mirror, $\bar\alpha z + \bar d(z)$ is the conjugate of $w = \alpha\bar z + d(\bar z) = \pi^v y$, and $\bar\pi = -i\pi$, so it equals $\pi^v(-i)^v\bar y$ with $(-i)^v\bar y$ odd. $\square$

*Theorem 6.4.* Let the map commute with the units. Then its rotations commute with the units, $S^n_{u\alpha}(z) = u^n S^n_\alpha(z)$, and each rotation is a Collatz map exactly when the original is. On $\mathbb{Z}[i]$ the mirrored map commutes with the units, $S^n_{\bar\alpha}(z) = (-i)^{V_n(\bar z)}\,\overline{S^n_\alpha(\bar z)}$ with $V_n$ the total exponent of the first $n$ steps, the norm sequence of $z$ under the mirror is that of $\bar z$ under the original, and the mirror is a Collatz map exactly when the original is.

*Proof.* For rotation, $d^u(vz) = d(uvz) = v\,d^u(z)$, and Proposition 6.3 with $S_\alpha(uz) = uS_\alpha(z)$ gives $S_{u\alpha} = uS_\alpha$, hence the iterates by induction. For the mirror, induction on $n$ from Proposition 6.3, using that the mirror commutes with the units to move the unit $(-i)^v$ out of each later step. In both cases the new map is a unit times a function $h$ that is a Collatz map exactly when the original is ($h = S_\alpha$, or $h(z) = \overline{S_\alpha(\bar z)}$), and a map that commutes with the units and agrees with $h$ up to a unit at each point is a Collatz map if $h$ is: its iterates are unit multiples of those of $h$, and a point $x$ with $g^p(x) = \varepsilon x$ satisfies $g^{p\,|U|}(x) = x$. $\square$

For $\mathbb{Z}[i]$ these are in Lean (`Expr.rotate_S`, `Expr.mirror_S`, `rotate_rule`, `mirror_rule`, `trivial_transfer`). Without the condition only Proposition 6.3 holds, and the computations show what is lost: the conjugate multipliers $2 - i$ and $2 + i$ get opposite verdicts at $(1+i)^2$, the associates $3i$ and $3$ do at $(1+i)^3$, and at $(1+i)^4$, among the $32$ Collatz tables with the smallest digits for the associates of $5 + 2i$ and the $16$ for those of $5 - 2i$, the counts differ between multipliers that rotation or mirroring would identify (computed to $10^5$). The even step is the reason: $\overline{z/(1+i)} = i\bar z/(1+i)$ introduces a unit, which a table that commutes with the units absorbs and any other table treats as a different input. The same remark shows that a map that does not commute with the units depends on the choice of the generator $1+i$ of its prime.

### 6.3 What we observe

Commuting with the units does not follow from being a Collatz map. In $\mathbb{Z}[i]$ every Collatz map at $(1+i)^2$ and $(1+i)^3$ fails to commute with the units, and at $(1+i)^4$, $27$ of the $35$ Collatz maps with the smallest digits do not commute with them (computed to $10^5$). In $\mathbb{Z}[\omega]$ non-commuting tie-breaks at modulus $8$ give Collatz maps where the commuting tables give none at $10^5$. What the condition is needed for is the symmetry of the theory: orbits that are unit multiples of each other, trivial cycles that treat the units alike, and rotated and mirrored maps that are Collatz maps together. In $\mathbb{Z}$ the question hardly arises, because from modulus $4$ the simplest maps have the property. In $\mathbb{Z}[i]$ it has a price: a Collatz map that commutes with the units first appears at norm $29$, at modulus $(1+i)^4$, and the companion paper [Paper 2] proves this bound and studies that map, $5 + 2i$.

## 7. Open questions

1. Is any map in the window a Collatz map? Every positive result here is a check to a finite bound, as for $3x+1$. This includes $7x \pm 1$, the $251$ integer maps of Table 1, the five maps at $(1+i)^2$, the $305$ at $(1+i)^3$, and $5 + 2\omega$.
2. Does $\alpha = 3$ admit a Collatz map at modulus $(1+i)^3$? Fixed points rule out no digit of norm at most $100$, so a proof of impossibility would need an argument about longer cycles.
3. Can divergence be proved for the real multipliers $5$, $9$, $11$ in $\mathbb{Z}[i]$ or $5$, $9$ in $\mathbb{Z}[\omega]$, by an exact description of the orbits on the invariant lines in place of the equidistribution heuristic?
4. Does the argument of Terras and Everett [Terras 1976; Everett 1977] carry over? The parity pattern of the first $n$ steps depends only on $z$ modulo a power of $\pi$, and we expect finite stopping time on a set of density one for every map below the upper end of the window. It would not exclude divergence, since Kionke's examples show that both can occur together [Kionke 2017].
5. Do the maps with many cycles have finitely many? This needs an effective lower bound for $|\varepsilon\pi^V - \alpha^n|/|\pi|^V$, a linear form in two logarithms for real $\alpha$, or in a logarithm and an angle otherwise, in the manner of Steiner and of Simons and de Weger for $3x+1$ [Steiner 1977; Simons and de Weger 2005].
6. Which other rings carry Collatz maps that commute with the units, at which modulus? Theorem 6.1 gives the thresholds for every imaginary quadratic field, and the maps there have not been computed. The Eisenstein maps that divide by $1 - \omega$ are untested.
7. Is there a map whose multiplier or modulus depends on the class (of the type of Matthews [Matthews 2010]) that restricts to $3x+1$ on the real line and has a single cycle? Such a map would need modulus at most $(1+i)^2$ on the class of $3$ modulo $4$. A single-cycle example would contain the Collatz conjecture, and by Conway's theorem such maps have to be studied one at a time.
8. Has the score $1/M - 1/N$, or an equivalent, been used before? We have not searched.

## 8. Miscellaneous

These items from the working notes bear on the subject but did not fit the thread.

1. Joseph's map on $\mathbb{Z}[i]$ has mean change $\frac{3}{4}\ln 9 - \ln 4 = +0.26$ of $\ln N$ per step, if the classes modulo $2$ are visited evenly. Of $314{,}197$ starts with $N(z) \le 10^5$, $98.2\%$ had not reached a cycle after $20{,}000$ steps or passed coordinates $2^{100}$, and the rest fell into $67$ cycles, not compared with Kucinski's list (computed). With $\alpha = 1 \pm 2i$ in place of $3$, all $16$ small choices of the added terms converge on $N(z) \le 10^5$ and $8$ have a single cycle (computed).
2. On the real axis the Gaussian map with $\alpha = 3$ and $D = U$ is the accelerated $3x+1$ map applied to whichever of $\pm x$ is $1$ modulo $4$, so every real orbit decreases to $1$; its other cycles lie off the axes. The multiplier $3$ is inert, so multiplication by it preserves the real line, while a split prime such as $2 + i$ leaves no line invariant (proved).
3. Below the window, maps are the counterpart of the $x+1$ problem: with $\alpha = 1 \pm 2i$ or $2 \pm i$ at $(1+i)^3$ and $D = U$, every orbit with $N(z) \le 10^5$ reached a unit within $20$ odd steps (computed; the contraction is proved).
4. Other maps on $\mathbb{Z}$ that commute with $-1$: Collatz's original permutation is odd [Tremblay 2019], Conway's amusical permutation satisfies $a(n) = -a(-n)$ [OEIS A006368], and Matthews's generalization $L_d$ of a map of Lu Pei, with least absolute residues, is odd for odd $d$ [Carnielli 2015; Tremblay 2019]. A check of ours, not reproducible from the repository, found that every $1 \le x < 10^5$ reaches $1$ under $L_3$.
5. Kohl's residue-class-wise affine maps include maps of $\mathbb{Z}^2$ with $2 \times 2$ integer matrix multipliers, and each step of a Gaussian map is one of them [Kohl 2005; unverified: no worked example on $\mathbb{Z}^2$ seen].
6. Kionke proposes integer matrix multipliers as an open extension of his lattice maps [Kionke 2017], and multiplication by a Gaussian $\alpha$ is of that kind. Matthews's example on $\mathbb{Z}[\sqrt 2]$ with the real multiplier $3$ has three invariant one-dimensional sets and apparently divergent orbits [Matthews 2010, Example 10.1], a precedent for the lines of Sections 4.3 and 5.2.
7. Siegel and O'Dwyer prove almost bounded orbits on a set of logarithmic density one for Hydra maps on rings of integers under a dimension constraint [Siegel and O'Dwyer 2026; unverified: the text seen was cut off]. Nichols proves a theorem of Terras type in function rings over $\mathbb{F}_2$ [Nichols 2016; unverified: abstract only].
8. Garg studies affine rules on Gaussian integers with dyadic denominators, which are not integral on $\mathbb{Z}[i]$ [Garg 2026; unverified: summary only], and Aliyev lists a Gaussian version of rational cycles as future work [Aliyev 2025].
9. Erdős published nothing on $3x+1$; the remark "hopeless" is reported from conversation, and the "mathematics is not ready" remark is not in Lagarias's account [Lagarias 2016].
10. Lagarias restates $3x+1$ through the semigroup of the inverse maps $2x$ and $(2x-1)/3$: the integers in the orbit of $4$ form the backward tree [Lagarias 2016; Klarner and Rado 1974]. [Paper 2] carries the restatement over to a Gaussian map.

## 9. Code and data

Everything is in `research/collatz/`. Integer maps: `zcz.c` (census and drop-below-start check, exact $128$-bit and multi-word arithmetic), `zany.c` (any digit table, literal cycles), `zcensus.py`, `zsummary.py`, `zverify.py` (independent check of two cycles per map), `zesc.py`, `zresonance.py` (the count $E$), `zsym.py`. Gaussian maps: `gcz.c` (full classification), `gstop.c` and `gstopall.c` (drop-below-start checks, the second over all quadrants with no condition on units), `m3search.c` and `m3search.py` (the search at $(1+i)^3$), `symtest.py`, `variants29.py`, `mirror29.py`, `rules.py`, `exact.py` (exact iteration, digits from the congruence), `sweep_gen.py`, `below_window.py`, `joseph.py`, and the line scripts `famlines.py`, `famline1d.py`, `famlinerun.py`. Eisenstein maps: `zeis.py`, `zeisline.py`, `eistest.py`. Logs are in `logs/`, among them `zsummary_sym.txt`, `zcensus_sym_drop_1e7.txt`, `zverify.txt`, `zresonance.txt`, `zcensus_pos_k2-10_B100000_mlt5over3.txt`, `symtest_Z_*`, `symtest_k2_B1e5.txt`, `symtest_k2_gstopall_1e7.txt`, `symtest_k4_B1e5.txt`, `m3search_*`, `zeis_k2_B1e5.txt`, `zeis_k2_drop.txt`, `zeis_k3_B1e5.txt`, `zeis_lines.txt` and `eistest_*`. The working notes are `integer-census.md`, `symmetry-test.md`, `theorems.md`, `modulus3-search.md` and `score.md`. The Lean 4 proofs are in `lean/` (core Lean, toolchain v4.35.0, no Mathlib, no `sorry`, `admit` or `native_decide`; the axioms of every main theorem are at most `propext`, `Classical.choice` and `Quot.sound`), built with `lake build` and checked with `lake env lean CheckAxioms.lean`. The Lean development covers $\mathbb{Z}[i]$ only. The check of $L_3$ in Section 8 was run from a scratchpad script that is not in the repository.

## References

- [Aliyev 2025] Aliyev, *Notes on Number Theory and Discrete Mathematics* 31(3) (2025), 658-666 [unverified: title and initials not recorded in the working notes].
- [Alvarado 2025] A. Alvarado, The Collatz conjecture over the Gaussian integers, talk slides, University of Colima, 2 April 2025, on work of a 2017 REU at CSU Channel Islands (NSF 1359165). https://fefoaranda.com/wp-content/uploads/2025/04/colima-alvarado-2025.pdf [unverified: slides only; the paper, said to be submitted, was not found].
- [Barina 2018] D. Barina, 7x ± 1: Close Relative of the Collatz Problem, arXiv:1807.00908 (2018); *Computational Methods in Science and Technology* 28(4) (2022), 143-147, doi:10.12921/cmst.2022.0000025.
- [Carnielli 2015] W. Carnielli, Some natural generalizations of the Collatz problem, *Applied Mathematics E-Notes* 15 (2015), 207-215.
- [Chamberland] M. Chamberland, An update on the $3x+1$ problem. https://chamberland.math.grinnell.edu/papers/3x_survey_eng.pdf
- [Collatz 1986] L. Collatz, On the motivation and origin of the $3x+1$ problem (in Chinese), *J. Qufu Normal University, Natural Science Edition* 12(3) (1986), 9-11; English translation in *The Ultimate Challenge: The $3x+1$ Problem*, American Mathematical Society, 2010, 241-247 [unverified: citation copied from Lagarias 2016].
- [Conway 1972] J. H. Conway, Unpredictable iterations, in *Proceedings of the 1972 Number Theory Conference*, University of Colorado, Boulder, 1972, 49-52 [unverified: citation copied from Lagarias 2016].
- [Coxeter 1971] H. S. M. Coxeter, Cyclic sequences and frieze patterns (The Fourth Felix Behrend Memorial Lecture), *Vinculum* 8 (1971), 4-7 [unverified: citation copied from Lagarias 2016].
- [Everett 1977] C. J. Everett, Iteration of the number-theoretic function $f(2n) = n$, $f(2n+1) = 3n+2$, *Advances in Mathematics* 25 (1977), 42-45 [unverified: from memory].
- [Garg 2026] A. Garg, Collatz-type dynamics on Gaussian integers: fractal stability islands, long cycles, and dyadic rational structure, Zenodo, doi:10.5281/zenodo.19078383 (March 2026) [unverified: summary only].
- [Hasse 1975] H. Hasse, *Unsolved Problems in Elementary Number Theory*, lectures, 1975 [unverified: known through Lagarias 2003].
- [Heppner 1978] E. Heppner, Eine Bemerkung zum Hasse-Syracuse-Algorithmus, *Archiv der Mathematik* 31 (1978) [unverified: known through Lagarias 2003; pages not recorded].
- [Joseph 1998] J. A. Joseph, A chaotic extension of the $3x+1$ function to $\mathbb{Z}_2[i]$, *Fibonacci Quarterly* 36(4) (1998), 309-316.
- [Kionke 2017] S. Kionke, A geometric approach to divergent points of higher dimensional Collatz mappings, arXiv:1511.05893 (2015); *Monatshefte für Mathematik* 182 (2017) [unverified: journal details from a secondary source].
- [Klarner and Rado 1974] D. A. Klarner and R. Rado, Arithmetic properties of certain recursively defined sets, *Pacific Journal of Mathematics* 53(2) (1974), 445-463 [unverified: citation copied from Lagarias 2016].
- [Kohl 2005] S. Kohl, *Restklassenweise affine Gruppen*, Dissertation, Universität Stuttgart, 2005; RCWA, a GAP package.
- [Kucinski 2000] G. I. Kucinski, Cycles for the $3x+1$ map on the Gaussian integers, preprint, University of Scranton, May 2000. https://monks.scranton.edu/files/pubs/kucinski.pdf
- [Lagarias 1985] J. C. Lagarias, The $3x+1$ problem and its generalizations, *American Mathematical Monthly* 92 (1985), 3-23.
- [Lagarias 2003] J. C. Lagarias, The $3x+1$ problem: an annotated bibliography (1963-1999), arXiv:math/0309224 [unverified: read up to about entry 100].
- [Lagarias 2006] J. C. Lagarias, The $3x+1$ problem: an annotated bibliography, II (2000-2009), arXiv:math/0608208.
- [Lagarias 2010] J. C. Lagarias, The $3x+1$ problem: an overview, in J. C. Lagarias (ed.), *The Ultimate Challenge: The $3x+1$ Problem*, American Mathematical Society, 2010, 3-29; arXiv:2111.02635.
- [Lagarias 2016] J. C. Lagarias, Erdős, Klarner, and the $3x+1$ problem, *American Mathematical Monthly* 123(8) (2016), 753-776, doi:10.4169/amer.math.monthly.123.08.753.
- [Matthews 2010] K. R. Matthews, Generalized $3x+1$ mappings: Markov chains and ergodic theory, in *The Ultimate Challenge: The $3x+1$ Problem*, American Mathematical Society, 2010, 79-103. http://www.numbertheory.org/PDFS/matthews-final-revised.pdf
- [Matthews, Barina's 7x+1 conjecture] K. R. Matthews, David Barina's 7x+1 conjecture, web page on numbertheory.org, linked from OEIS A317640 [unverified: not reached].
- [Matthews and Watts 1984] K. R. Matthews and A. M. Watts, A generalization of Hasse's generalization of the Syracuse algorithm, *Acta Arithmetica* 43 (1984), 167-175.
- [Möller 1978] H. Möller, Über Hasses Verallgemeinerung des Syracuse-Algorithmus (Kakutanis Problem), *Acta Arithmetica* 34 (1978), 219-226.
- [Nichols 2016] D. Nichols, Analogues of the $3x+1$ problem in polynomial rings of characteristic $2$, *Experimental Mathematics* (2016), arXiv:1610.02545.
- [OEIS A006368] The amusical permutation of the nonnegative numbers (J. H. Conway), The On-Line Encyclopedia of Integer Sequences, https://oeis.org/A006368.
- [OEIS A037084] Positive integers that do not reach 1 under the 3n-1 map, https://oeis.org/A037084 [unverified: title paraphrased from the notes].
- [OEIS A305057] A. del Arte, -5x + 1 sequence starting at 5, May 2018, https://oeis.org/A305057.
- [OEIS A317640] D. Barina, The 7x+-1 function, 2018, https://oeis.org/A317640.
- [OEIS A317753] D. Barina, Number of steps to reach 1 in the 7x+-1 problem, 2018, https://oeis.org/A317753.
- [OEIS A318489] D. Barina, Dropping time in the 7x+-1 problem, 2018, https://oeis.org/A318489.
- [OEIS A318972] D. Barina, The 7x+-1 function in shortcut form, 2018, https://oeis.org/A318972.
- [Paper 2] The 5+2i problem: Collatz maps on the Gaussian integers, companion paper, `papers/collatz-5p2i.md` (in preparation).
- [Siegel 2021] M. C. Siegel, Functional equations associated to Collatz-type maps on integer rings of algebraic number fields, arXiv:2111.07882 (withdrawn).
- [Siegel 2024] M. C. Siegel, $(p,q)$-adic analysis and the Collatz conjecture, arXiv:2412.02902.
- [Siegel and O'Dwyer 2026] M. Siegel and R. O'Dwyer, Almost-boundedness of Collatz-type maps on number rings via Archimedean methods, arXiv:2609.33018 (2026).
- [Simons and de Weger 2005] J. Simons and B. de Weger, Theoretical and computational bounds for $m$-cycles of the $3n+1$ problem, *Acta Arithmetica* 117 (2005), 51-70 [unverified: from memory].
- [Steiner 1977] R. P. Steiner, A theorem on the Syracuse problem, *Proceedings of the 7th Manitoba Conference on Numerical Mathematics* (1977), 553-559 [unverified: from memory].
- [Terras 1976] R. Terras, A stopping time problem on the positive integers, *Acta Arithmetica* 30 (1976), 241-252 [unverified: from memory].
- [Tremblay 2019] R. Tremblay, Generalized $3x+1$ mappings: searching for cycles, arXiv:1907.13302.
