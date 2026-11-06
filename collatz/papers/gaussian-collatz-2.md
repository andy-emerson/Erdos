# Collatz maps on the Gaussian integers

*Second draft, 4 November 2026. Code, logs and Lean proofs: `collatz/research/`. Items marked [unverified] have not been checked against their source or by computation.*

## Abstract

We study maps on the Gaussian integers built like the $3x+1$ map, with the prime $1+i$ in the role of $2$. An element divisible by $1+i$ is divided by it; any other $z$ is sent to $\alpha z + d$, where the digit $d$ is chosen by the class of $z$ modulo $(1+i)^k$ so that the result is divisible by $(1+i)^k$. We ask three things of such a rule: that it commute with multiplication by units, so that it treats $z$ and $-z$ alike, which $3x+1$ does not; that its digits be as small as possible; and that it expand in the worst case and contract on average, which happens exactly when $2^k < N(\alpha) < 2^{k+1}$. No such rule exists at levels $k \le 2$. Ordered by the norm of $\alpha$, the first such rule with a single cycle up to units is $H$, with $\alpha = 5+2i$, $k = 4$ and digits $\pm 1, \pm i, \pm(2+i), \pm(1-2i)$. It is the only one at level $4$ up to conjugation; levels $3$ and $5$ have none, and levels $6$ and $7$ have $22$ of $112$ and $8$ of $46$. We conjecture that every orbit of $H$ on $\mathbb{Z}[i] \setminus \{0\}$ reaches $\pm 1$ or $\pm i$, and we have checked this for all starts of norm at most $3 \cdot 10^9$. The multiplier $3$ falls at level $3$, where its rule has five cycles, and no rule with $\alpha = 3$ both agrees with $3x+1$ on the positive integers and contracts on average. The soundness of the check, the facts about $H$ that it uses, and three of the elementary propositions are proved in Lean 4.

## 1. Introduction

The $3x+1$ map sends an even integer $x$ to $x/2$ and an odd one to $3x+1$. The Collatz conjecture says that every positive integer reaches $1$ [1]. The map has an accelerated form on odd numbers, $x \mapsto (3x+1)/2^v$ with $2^v$ the largest power of $2$ dividing $3x+1$. Each odd step multiplies $x$ by about $3$ and is followed by at least one division by $2$, so in the worst case $x$ grows by a factor $3/2$. For a random odd $x$ the number of divisions has mean $2$, so on average $x$ shrinks by a factor $3/4$. The problem lives in this gap between expansion in the worst case and contraction on average.

The same map makes sense on all of $\mathbb{Z}$, and there it has more cycles. Besides the fixed point $0$ and the cycle $1 \to 4 \to 2$, it has the cycle $-1 \to -2$ of $2$ steps, one through $-5$ of $5$ steps and one through $-17$ of $18$ steps. None of these is the negative of $1 \to 4 \to 2$. The reason is that $3x+1$ does not commute with $-1$: $-(3x+1) = 3(-x) - 1$, so on the negative integers the map acts as $3x-1$ acts on the positive ones, and $3x-1$ has three cycles. The single cycle conjectured for $3x+1$ is a property of one half of $\mathbb{Z} \setminus \{0\}$.

A conjecture of Collatz type can fail in two ways: some orbit falls into a second cycle, or some orbit grows without bound. Its content is that there is a single cycle and that every orbit reaches it. The example of $\mathbb{Z}$ suggests two requirements for a rule that is meant to carry such a conjecture on a ring larger than $\mathbb{N}$. The rule should commute with the units of the ring, so that no half of the ring is singled out, and its cycles should be counted up to units.

This paper asks what the natural analogue of $3x+1$ on the Gaussian integers is. The prime $1+i$, of norm $2$, plays the part of $2$, and a rule multiplies by a Gaussian integer $\alpha$ and adds a digit from a fixed set. Section 3 states three criteria for such a rule (symmetry under units, minimal digits, and the window $2^k < N(\alpha) < 2^{k+1}$ in which it expands in the worst case and contracts on average) and orders the rules by the norm of $\alpha$. The first rule in this order with a single cycle up to units is

$$H(z) = \begin{cases} z/(1+i) & \text{if } (1+i) \mid z, \\ (5+2i)z + d(z) & \text{otherwise,} \end{cases}$$

where $d(z)$ is the element of $\{\pm 1, \pm i, \pm(2+i), \pm(1-2i)\}$ that makes $(5+2i)z + d(z)$ divisible by $4$.

*Conjecture 1.* Every orbit of $H$ on $\mathbb{Z}[i] \setminus \{0\}$ reaches $\pm 1$ or $\pm i$. Up to units, $H$ has a single cycle.

On $\mathbb{Z}[i]$ itself the cycles are the two through $\pm 1$ and through $\pm i$, each the image of the other under multiplication by $i$. We have checked Conjecture 1 for every start of norm at most $3 \cdot 10^9$ (Section 7). The check rests on a reduction to one starting point per class of associates. That reduction and the facts about $H$ it needs are proved in Lean 4 without external libraries, as are Propositions 2 and 4 and part of Proposition 1 (Section 8).

Section 2 surveys prior work. Section 3 sets up the family and the criteria, and Section 4 proves the basic facts about them. Section 5 is a census of the rules at levels $3$ to $7$. Section 6 studies $H$, Section 7 the computation, Section 8 the formal proofs, and Section 9 lists open questions.

## 2. Prior work

Lagarias's annotated bibliographies [2, 3] record the generalizations of $3x+1$ to other rings. We read them in part [unverified: bibliography I was read up to about entry 100, and bibliography II in its second version in full and in its current version up to entry 89].

The family studied here is a case of several general frameworks. Matthews [4, Section 10] considers generalized $3x+1$ maps on the ring of integers $\mathcal{O}_K$ of a number field, taken modulo a non-unit $d$, and works examples on $\mathbb{Z}[\sqrt 2]$ and $\mathbb{Z}[\sqrt 3]$; our rules are maps of this kind on $\mathbb{Z}[i]$ with $d = (1+i)^k$, though $\mathbb{Z}[i]$ is not among his examples. As reported by Siegel [5], Leigh was the first to consider Collatz-type maps on rings of algebraic integers, and Matthews knew of no other work on them [unverified: Leigh's work not seen]. Kohl's residue-class-wise affine maps [6] include maps of $\mathbb{Z}^2$ whose pieces have $2 \times 2$ integer matrix multipliers and lattice moduli, and each step of our rules is such a map [unverified: taken from the package source; no worked $\mathbb{Z}^2$ example seen]. Siegel's Hydra maps [5] cover maps on rings of integers in general, without examples in $\mathbb{Z}[i]$. Conway [7] showed in 1972 that a generalization of $3x+1$ to residue-class-wise affine maps of $\mathbb{Z}$ is undecidable [unverified: known through [14]], so questions about such maps in general have no algorithmic answer and must be settled rule by rule.

Kionke [8] studies maps on lattices $\mathbb{Z}^e$ and on $\mathbb{Z}[\sqrt 2]$ of the form $x \mapsto (m_w x + r_w)/d$ on each coset, with positive integer multipliers $m_w$. He proves that tame cones give divergent orbits of positive lower density, that on $\mathbb{Z}[\sqrt 2]$ at least half of all points diverge, and that a map can have finite stopping time on a set of density one and still diverge on a set of positive density. His multipliers are scalars, and he proposes integer matrices in their place as an open extension. Multiplication by a Gaussian $\alpha$ followed by division by a power of $1+i$ is of that matrix kind, and it rotates as well as stretches.

Joseph [9] defined a map on the $2$-adic Gaussian integers that sends $z$ to $z/2$ on the class $0$ modulo $2$ and to $(3z + \beta)/2$ on the class $\beta \in \{1, i, 1+i\}$. It restricts to the $3x+1$ map on $\mathbb{Z}_2$, and Joseph proved it chaotic in the sense of Devaney and topologically conjugate to the product of the $2$-adic $3x+1$ map with itself. Kucinski [10] studied Joseph's map on $\mathbb{Z}[i]$: she bounded the period of a cycle in terms of its parity vector, showed that the second and fourth quadrants contain no cyclic points, and found all $77$ cycles of period at most $400$ [unverified: read through a text extraction]. Chamberland's survey [11] records both. Joseph's map divides by $2$, and its mean change of $\ln N$ per step, if the classes modulo $2$ are visited evenly, is $\frac{3}{4} \ln 9 - \ln 4 = +0.26$. In our runs $98.2\%$ of the starts with $N(z) \le 10^5$ either had not reached a cycle after $20000$ steps or had a coordinate pass $2^{100}$, and the rest fell into $67$ cycles, which we have not compared with Kucinski's list.

Alvarado and the students of a 2017 REU at CSU Channel Islands [12] studied the rule that sends $z$ to $z/(1+i)$ if $1+i$ divides $z$ and to $(2+i)z + u$ otherwise, with $u$ a fixed unit. It is the rule of our family at level $1$ with $\alpha = 2+i$. They report that about $5.6\%$ of the starts with $|a|, |b| \le 100$ reached a unit within $10000$ steps [unverified: seen only in talk slides; the paper, said to be submitted, was not found]. Section 4 explains this: $N(2+i) = 5$ lies above the level-$1$ window, so the rule expands on average.

Klarner and Rado [13] introduced the orbits of semigroups of integer affine maps, and Lagarias [14] used their language to restate $3x+1$: the backward tree of $1$ is the integral part of the orbit of $4$ under the inverse maps $2x$ and $(2x-1)/3$. Section 6 gives the same restatement for $H$. Siegel and O'Dwyer [15] extend Tao's theorem on almost bounded orbits to Hydra maps on rings of integers under a dimension constraint; we have not checked whether $H$ or an iterate of it satisfies their hypotheses [unverified: the text seen was cut off before the references]. Garg [16] studies affine rules on four classes of Gaussian integers with dyadic denominators, which are not integral on $\mathbb{Z}[i]$ [unverified: seen only through a summary], and Aliyev [17] lists a Gaussian version of rational cycles as future work.

As far as our search reached, no earlier work treats rules on $\mathbb{Z}[i]$ that divide by $1+i$ at levels $k \ge 2$, takes the level and the digit set as parameters, or asks which such rules have a single cycle [unverified: parts of [2] and several web pages could not be read]. The window, the census and the rule $H$ are new in that sense. The window's upper end is the standard growth-rate heuristic carried over to the norm (Section 3).

## 3. The framework

### 3.1 Levels, digits and the accelerated map

The Gaussian integers are the numbers $a+bi$ with $a, b \in \mathbb{Z}$, with norm $N(a+bi) = a^2+b^2$ and units $\pm 1, \pm i$. Two Gaussian integers are associates if one is a unit times the other. Factorization into primes is unique up to units. The prime $2$ is not a Gaussian prime: $2 = -i(1+i)^2$, and $1+i$, of norm $2$, plays the part of $2$. The ring $\mathbb{Z}[i]/(1+i)$ has two elements. We call $z$ **even** if $1+i$ divides $z$ and **odd** otherwise. Since

$$\frac{a+bi}{1+i} = \frac{(a+b) + (b-a)i}{2},$$

$a+bi$ is even exactly when $a+b$ is even, and dividing by $1+i$ halves the norm. For an integer $n$ the exponent of $1+i$ in $n$ is twice the exponent of $2$.

Fix an odd $\alpha$, the multiplier, and an integer $k \ge 0$, the **level**. For odd $z$ we want a digit $d(z)$ with

$$(1+i)^k \mid \alpha z + d(z),$$

that is, $d(z) \equiv -\alpha z \pmod{(1+i)^k}$. The right-hand side depends only on the class of $z$ modulo $(1+i)^k$. The ring $\mathbb{Z}[i]/(1+i)^k$ has $2^k$ elements, half of them odd for $k \ge 1$, and since $\alpha$ is invertible modulo $(1+i)^k$, the map $z \mapsto -\alpha z$ permutes the odd classes. A **digit set** $D$ is a set of odd Gaussian integers with one element in each odd class, $2^{k-1}$ in all, and $d(z)$ is the element of $D$ in the class of $-\alpha z$. The rule is

$$T(z) = \begin{cases} z/(1+i) & \text{if } (1+i) \mid z, \\ \alpha z + d(z) & \text{otherwise.} \end{cases}$$

Write $v(w)$ for the exponent of $1+i$ in $w \ne 0$. The accelerated map on odd $z$ is

$$S(z) = \frac{\alpha z + d(z)}{(1+i)^{v(\alpha z + d(z))}},$$

which is odd, and $v(\alpha z + d(z)) \ge k$ by construction. In $\mathbb{Z}$ the same construction with $2$ in place of $1+i$ gives $3x+1$ at level $1$ with the single digit $1$.

### 3.2 Symmetry

A rule is **symmetric** if $T(\varepsilon z) = \varepsilon T(z)$ for every unit $\varepsilon$. This holds exactly when $D$ is closed under multiplication by units, since $\varepsilon d(z)$ is then the digit for $\varepsilon z$. The orbit of $\varepsilon z$ under a symmetric rule is $\varepsilon$ times the orbit of $z$, so the rule acts on classes of associates, and it is enough to study one representative of each class, for instance the one with $a > 0$ and $b \ge 0$. Cycles come in sets of four associates, or fewer when a cycle contains associates of its own elements, and we count them up to units. If $D$ is also closed under complex conjugation, then $(\alpha, D)$ and $(\bar\alpha, D)$ give mirror-image rules, and in general $(\alpha, D)$ and $(\bar\alpha, \bar D)$ do.

Symmetry is what gives the single-cycle question its weight. Rules that are not symmetric often have a single cycle. In sweeps with $N(z) \le 10^5$ and each digit chosen freely among odd elements of norm at most $5$, a single nonzero cycle appeared in $68$ of $288$ rules at level $2$ (with $\alpha$ an associate of $2 \pm i$), $108$ of $972$ at level $3$ and $76$ of $448$ at level $4$. Section 4 shows that symmetric rules exist only from level $3$ on.

### 3.3 Minimal digits

For $k \ge 3$ the four units lie in distinct classes modulo $(1+i)^k$, since their differences have $v \le 2$, so the units act freely on the odd classes and divide them into $2^{k-3}$ orbits of four. A symmetric digit set is the set of unit multiples of one digit chosen for each orbit. We call a symmetric digit set **minimal** if each chosen digit has the least norm in its class. Smaller digits are preferred because the digit is the part of the rule that is arbitrary, and minimality is the order we use on digit sets. Section 4 lists the minimal sets.

### 3.4 The window

An odd step multiplies the norm by about $N(\alpha)$, and each of the at least $k$ divisions that follow halves it. If $N(\alpha) < 2^k$, every odd step contracts by at least a fixed factor once $|z|$ is large, and every orbit is eventually periodic (Proposition 3 in Section 4). Such rules are settled by a finite computation. If $N(\alpha) > 2^k$, an odd step with exactly $k$ divisions multiplies the norm by $N(\alpha)/2^k > 1$, and the rule expands in the worst case.

On average there are more divisions. Fix $j \ge 0$. As $z$ runs over the odd classes modulo $(1+i)^{k+j}$, the value $\alpha z + d(z)$ runs evenly over the classes of multiples of $(1+i)^k$ modulo $(1+i)^{k+j}$, and a fraction $2^{-j}$ of them are $0$. So in the sense of density over odd $z$, $v(\alpha z + d(z)) - k$ has the geometric distribution $P(v - k \ge j) = 2^{-j}$, with mean $1$ and variance $2$. If an orbit visits the classes as if at random, the mean change per odd step is

$$\mathbb{E}[\Delta \ln N] = \ln N(\alpha) - (k+1) \ln 2,$$

which is negative exactly when $N(\alpha) < 2^{k+1}$. This is the norm form of the growth-rate heuristic of Matthews and Watts [18], which generalizes conjectures of Möller [19]: for a map $x \mapsto (m_i x - r_i)/d$ on the classes $i$ modulo $d$ of $\mathbb{Z}$, almost all trajectories are expected to cycle if $|m_0 \cdots m_{d-1}| < d^d$ and to diverge if the product exceeds $d^d$ [4, Conjecture 3.1]. We call a rule **Collatz-like** if

$$2^k < N(\alpha) < 2^{k+1},$$

so that it expands in the worst case and contracts on average. For $3x+1$ the criterion reads $2 < 3 < 4$ at level $1$.

### 3.5 The order on rules

We compare Collatz-like rules by the norm of $\alpha$. The windows $(2^k, 2^{k+1})$ are disjoint, so the norm of a Collatz-like multiplier determines its level, and a rule at a lower level has a smaller norm than every rule at a higher level. Ordering by level and then by norm therefore gives the same order as ordering by norm alone, and the same first rule with a single cycle. Among rules with the same multiplier, minimal digit sets come first. With this order, the question "which is the natural analogue of $3x+1$" becomes "which is the first symmetric, minimal, Collatz-like rule with a single cycle up to units", and Section 5 finds that it is $H$, at level $4$ and norm $29$, unique there up to conjugation. Its uniqueness is a computational statement for the other rules, each of which has at least two cycles (Proposition 5), and a conjecture for $H$ itself.

### 3.6 The parallel in $\mathbb{Z}$

The same criteria can be applied in $\mathbb{Z}$, with $2$ in place of $1+i$ and $-1$ as the only nontrivial unit. A rule at level $k$ has digits for the odd classes modulo $2^k$ and is Collatz-like when $2^k < m < 2^{k+1}$ for its multiplier $m$. The multiplier $3$ forces level $1$, where there is a single odd class and $x \equiv -x \pmod 2$, so $x$ and $-x$ receive the same digit and no rule commutes with $-1$. This is why $3x+1$ is not symmetric. Symmetric integer rules exist from level $2$ on, with multiplier $5$ or $7$ and digits $\pm 1$: the digit is $-1$ for $x \equiv 1 \pmod 4$ and $+1$ for $x \equiv 3$ when $m = 5$, and the reverse when $m = 7$. We followed every odd start with $|x| \le 10^5$ under both (script `zsym.py`). The rule $5x \pm 1$ has two cycles up to sign, the fixed point $1$ and $7 \to 9 \to 11$. Under $7x \pm 1$ every odd $x$ with $3 \le x \le 10^7$ falls below $x$ in absolute value, so every odd $x$ with $|x| \le 10^7$ reaches the fixed point $1$ or $-1$. This is an observation up to that bound. Integer rules of this kind may already appear in the literature on generalized maps [4, 2, 3], which we have not searched for them. Up to $10^7$, then, the first symmetric Collatz-like rule in $\mathbb{Z}$ with a single cycle, in the order of Section 3.5, is $7x \pm 1$, at the lowest level that has symmetric rules. In $\mathbb{Z}[i]$ that level is $3$, where no rule has a single cycle, and the first is $H$ at level $4$. In both rings the first single-cycle rule has the largest multiplier at its level.

### 3.7 The multiplier $3$

The Gaussian multiplier $3$ has norm $9$, which lies in the window $(8, 16)$ of level $3$. At level $3$ the four units form a complete set of odd residues, so there is exactly one minimal digit set, the units, and one rule:

$$G_3(z) = \begin{cases} z/(1+i) & \text{if } (1+i) \mid z, \\ 3z + u(z) & \text{otherwise,} \end{cases}$$

with $u(z)$ the unit that makes $3z + u(z)$ divisible by $(1+i)^3$. Its mean factor per odd step, $9/16 = (3/4)^2$, is the square of the factor for $3x+1$. All starts with $N(z) \le 10^9$ reach one of five cycles up to units: the fixed class of $1$, two cycles of two odd steps through $2+i$ and $5+2i$, and a mirror pair of cycles of five odd steps through $19+6i$ and $6+19i$. Of the starts with $N(z) \le 10^6$, $92.3\%$ end in the cycle through $2+i$ and only $0.2\%$ at $1$. In this respect $G_3$ resembles $3x+1$ on all of $\mathbb{Z}$.

On the real line $G_3$ is an easy form of $3x+1$. Let $x$ be an odd integer. An integer divisible by $(1+i)^3$ is divisible by $4$, since the exponent of $1+i$ in an integer is even. A non-real unit gives $v(3x \pm i) = 1$, so $u(x) = \pm 1$, and up to sign $3x + u = 3y + 1$, where $y$ is whichever of $\pm x$ is $1 \bmod 4$. So on the axes, up to units, $G_3$ is the accelerated $3x+1$ map applied to that representative. Since $y \equiv 1 \pmod 4$, the number $3y+1$ is divisible by $4$, and $|S(x)| \le (3|x|+1)/4 < |x|$ for $|x| > 1$. Every real orbit therefore decreases to $1$, and the four other cycles lie off the axes. The arithmetic of $3$ allows this: $3$ is inert in $\mathbb{Z}[i]$, so multiplication by $3$ preserves the real line, while a split prime such as $5+2i$ leaves no line invariant.

The line in $G_3$ does not carry the Collatz problem, and within the family no line can.

*Proposition 1.* Let a rule have $\alpha = 3$ and level $k$.

1. If $k \ge 3$ and every odd integer receives an integer digit, then on the axes, up to units, $|S(x)| < |x|$ whenever $|x|$ exceeds the largest such digit, so every real orbit is eventually periodic.
2. If the odd step at every positive odd integer $n$ is $n \mapsto 3n+1$, then $k \le 2$, and the rule is not Collatz-like, since $N(3) = 9 > 2^{k+1}$.

*Proof.* For (1), an integer divisible by $(1+i)^3$ is divisible by $4$, so $|S(x)| \le (3|x| + |d|)/4$, which is less than $|x|$ when $|x| > |d|$. For (2), $(1+i)^k$ divides $3 \cdot 3 + 1 = 10 = -i(1+i)^2(2+i)(2-i)$, so $k \le 2$, and then $2^{k+1} \le 8 < 9$. $\square$

So no rule in the family both agrees with $3x+1$ on the positive integers and contracts on average in the plane. Joseph's map escapes this by dividing by $2$, and as part (2) leads one to expect, it expands on average (Section 2). A rule whose multiplier or level depends on the class could still restrict to $3x+1$; Section 9 returns to this.

### 3.8 Below the window

Without the lower end of the window, single-cycle rules are easy to find and easy to prove. Take $\alpha = 1+2i$, of norm $5 < 8$, at level $3$ with the units as digits. For odd $z$,

$$|S(z)| \le \frac{\sqrt 5\, |z| + 1}{2^{3/2}},$$

which is less than $|z|$ once $|z| > 1/(2^{3/2} - \sqrt 5) \approx 1.69$. Every odd non-unit has norm at least $5$, so every odd step from a non-unit lowers the norm, and every orbit reaches a unit. From $1$ the digit is $1$, and $(1+2i) + 1 = 2+2i = -i(1+i)^3$, so $S(1) = -i$. By symmetry $S(-i) = -1$, $S(-1) = i$ and $S(i) = 1$, so the rule has exactly one cycle on $\mathbb{Z}[i] \setminus \{0\}$, through all four units. We confirmed this with the exact-arithmetic code of Section 7 (script `below_window.py`) for every odd start with $N(z) \le 10^5$, for $\alpha = 1+2i$, $2+i$, $1-2i$ and $2-i$: every odd step lowered the norm, every orbit reached a unit within $20$ odd steps, and the only cycle up to units is the class of $1$. Such a rule is the Gaussian counterpart of the $x+1$ problem, which Lagarias [14] calls the solved cousin of $3x+1$.

## 4. Basic results

### 4.1 Symmetry needs level $3$

*Proposition 2.* For $k \le 2$ and any odd $\alpha$, no rule at level $k$ is symmetric. At level $0$ the only symmetric rules send odd $z$ to $\alpha z$.

*Proof.* Since $-1 - 1 = -2 = i(1+i)^2$, we have $-z \equiv z \pmod{(1+i)^2}$ for every $z$. For $1 \le k \le 2$, $z$ and $-z$ then lie in the same class modulo $(1+i)^k$ and receive the same digit. Symmetry requires $d(-z) = -d(z)$, so $d(z) = 0$, which is not odd. At level $0$ the condition $(1+i)^0 \mid \alpha z + d$ holds for every $d$ and there is a single class, so a rule has a single added term; the only one-element set closed under units is $\{0\}$. $\square$

At level $0$ no odd orbit of a symmetric rule ever divides by $1+i$, since $\alpha z$ is odd, and the window $1 < N(\alpha) < 2$ contains no integer, so level $0$ is trivial. At level $2$ a weaker symmetry also fails: $S(-z)$ cannot always be an associate of $S(z)$. Since $z$ and $-z$ share a digit $d$, this would make $(\alpha z + d)/(\alpha z - d)$ a unit times a power of $1+i$. For large $|z|$ this ratio is within $1/2$ of $1$ without equalling it, and no unit times a power of $1+i$ other than $1$ lies within $1/2$ of $1$.

### 4.2 Below the window

*Proposition 3.* If $N(\alpha) < 2^k$, every orbit of $T$ is eventually periodic.

*Proof.* Let $\rho = |\alpha|/2^{k/2} < 1$ and $\delta = \max_{d \in D} |d|$. For odd $z$, $|S(z)| \le (|\alpha|\,|z| + \delta)/2^{k/2} = \rho|z| + \delta/2^{k/2}$. Let $R = \delta/\left(2^{k/2}(1-\rho)\right)$. If $|z| > R$ then $|S(z)| < |z|$, and if $|z| \le R$ then $|S(z)| \le R$. Even steps reduce $|z|$. Norms are nonnegative integers and cannot decrease forever, so every orbit enters the disc $|z| \le R$ and stays in a bounded set from then on. A bounded set of Gaussian integers is finite, so the orbit is eventually periodic. $\square$

### 4.3 Level $1$

*Proposition 4.* There is no Collatz-like rule at level $1$.

*Proof.* The window is $2 < N(\alpha) < 4$, so $N(\alpha) = 3$. A square is $0$ or $1$ modulo $4$, so a sum of two squares is $0$, $1$ or $2$ modulo $4$, and $3$ is not a norm. $\square$

The direct analogue of $3x+1$, with one odd class and one digit, does not exist in $\mathbb{Z}[i]$, for the same reason that $3$ stays prime there. The REU rule of Section 2 has $N(2+i) = 5$, above the window $(2, 4)$, with drift $\ln 5 - 2 \ln 2 > 0$ per odd step, which fits the $5.6\%$ of starts that reached a unit.

### 4.4 The multipliers and digit sets

Since $\alpha$ is odd, $N(\alpha) \equiv 1 \pmod 4$. The Collatz-like multipliers at each level are these, counted up to units:

| $k$ | window | odd norms in window | $\alpha$ up to units |
|---|---|---|---|
| 0 | $(1, 2)$ | none | none |
| 1 | $(2, 4)$ | none | none |
| 2 | $(4, 8)$ | 5 | $2 \pm i$ |
| 3 | $(8, 16)$ | 9, 13 | $3$, $3 \pm 2i$ |
| 4 | $(16, 32)$ | 17, 25, 29 | $4 \pm i$, $5$, $4 \pm 3i$, $5 \pm 2i$ |
| 5 | $(32, 64)$ | 37, 41, 45, 49, 53, 61 | 11 values |
| 6 | $(64, 128)$ | 65, 73, 81, 85, 89, 97, 101, 109, 113, 117, 121, 125 | 28 values |
| 7 | $(128, 256)$ | 18 norms, 137 to 245 | 46 values |

At level $2$ the multiplier $2+i$ has the smallest odd prime norm, which makes it the closest analogue of $3$, but by Proposition 2 none of its rules is symmetric.

The minimal symmetric digit sets of Section 3.3 are as follows. At level $3$ the units are a complete set of odd residues, so the digit set is the units. At level $4$, where classes modulo $(1+i)^4 = -4$ are classes modulo $4$, there are two orbits of classes: the classes of the units, and the classes of the unit multiples of $1+2i$, each of which contains two elements of norm $5$. The minimal sets are the units with the unit multiples of $2+i$, and the units with the unit multiples of $1+2i$; they are complex conjugates. At level $5$ the minimal set is unique: the unit multiples of $1$, $3$, $1+2i$ and $1-2i$. At level $6$ there are eight orbits and four minimal sets: the unit multiples of $1$, $3$, $1 \pm 2i$ and $2 \pm 3i$, together with those of $1+4i$ or $1-4i$ and of $3+4i$ or $3-4i$. None of the four is closed under conjugation. At level $7$ there are sixteen orbits, each class has a unique element of least norm, and the minimal set is unique: the unit multiples of $1, 3, 5, 7, 1 \pm 2i, 1 \pm 4i, 1 \pm 6i, 2 \pm 3i, 2 \pm 5i$ and $3 \pm 4i$. It is closed under conjugation.

Propositions 1(2), 2 and 4 are proved in Lean (Section 8). Propositions 1(1) and 3 are not.

## 5. The census

### 5.1 Method and summary

A rule in the census is symmetric, minimal and Collatz-like, and every choice of minimal digit set is counted. Rules are counted up to units, which replace $\alpha$ by an associate, and up to conjugation, which replaces $(\alpha, D)$ by $(\bar\alpha, \bar D)$; cycles are counted up to units. For each rule the program `gcz` (Section 7) followed every start with $N(z) \le B$ to its cycle, and every cycle it reported was iterated again in exact integer arithmetic, with each digit chosen from the congruence instead of from a table, before cycles were grouped up to units. A cycle in the census is one reached from some start with $N(z) \le B$, so the counts of cycles are lower bounds, except for the four level-$7$ rules whose lists Section 5.6 shows to be complete up to a stated length.

| $k$ | rules (up to conjugation) | single cycle (up to conjugation) | cycles of the other rules, up to conjugation | checked to |
|---|---|---|---|---|
| 0 to 2 | 0 | none | none | proof |
| 3 | 3 (2) | 0 | 5, 4 | $10^9$ |
| 4 | 14 (7) | 2 (1): $H$ | 2, 2, 3, 3, 6; $\alpha = 5$: 35 or more, divergent | $10^6$; $H$ to $3 \cdot 10^9$ |
| 5 | 11 (6) | 0 | 2, 3, 3, 4, 4, 18 | $10^6$ |
| 6 | 112 (56) | 22 (11) | 2 to 10; $\alpha = 9$: 23, 25; $\alpha = 11$: 560, 568 (at least 639 for one digit set); both divergent | $10^6$ |
| 7 | 46 (24) | 8 (4) | 2 to 33; $\alpha = 13, 15, 11 \pm 10i$: 166, 74, 766 | $10^6$; single-cycle rules and the four rules of Section 5.6 to $10^9$ |

"Checked to" is the bound on $N(z)$ up to which every start was followed to its cycle, except where starts escaped (Section 5.5). Level $4$ is the first level with a single-cycle rule, $H$ is the only one there up to conjugation, levels $3$ and $5$ have none, and levels $6$ and $7$ have $22$ of $112$ and $8$ of $46$. Since every rule at levels $3$ and $4$ with smaller norm than $H$ has at least two cycles, $H$ is the first single-cycle rule in the order of Section 3.5. Single-cycle rules are common from level $6$ on, so what distinguishes $H$ is that it comes first.

### 5.2 Levels $3$ to $5$

The table lists each rule with $k \le 5$ up to conjugation, with two of its cycles, each given by a representative in the first quadrant and its length in odd steps.

| $k$ | $\alpha$ | cycles found | two cycles (odd length) |
|---|---|---|---|
| 3 | $3$ | 5 | $1$ (1), $2+i$ (2) |
| 3 | $3+2i$ | 4 | $1$ (1), $3$ (4) |
| 4 | $4+i$ | 2 | $1$ (1), $2+i$ (1) |
| 4 | $4-i$ | 2 | $1$ (1), $2+i$ (1) |
| 4 | $5$ | 35 or more, and divergent orbits | $1$ (1), $2+i$ (1) |
| 4 | $4+3i$ | 3 | $1$ (1), $6+3i$ (11) |
| 4 | $4-3i$ | 6 | $1$ (1), $4+i$ (12) |
| 4 | $5+2i$ | 1 | $1$ (1) only |
| 4 | $5-2i$ | 3 | $1$ (1), $3+4i$ (6) |
| 5 | $6+i$ | 2 | $1$ (1), $5$ (4) |
| 5 | $5+4i$ | 4 | $1$ (1), $2+i$ (1) |
| 5 | $6+3i$ | 3 | $1$ (1), $2+i$ (2) |
| 5 | $7$ | 18 | $1$ (1), $2+i$ (1) |
| 5 | $7+2i$ | 4 | $1$ (1), $2+i$ (3) |
| 5 | $6+5i$ | 3 | $1$ (1), $2+i$ (7) |

At level $4$ the digit set in each row is the units with the unit multiples of $2+i$; the other minimal set gives the mirror images. At level $5$ the rules with $\alpha = 6-i$, $5-4i$, $6-3i$, $7-2i$ and $6-5i$ are the mirror images of those listed, and $\alpha = 7$ is its own. The counts are for all starts with $N(z) \le 10^6$, and apart from $\alpha = 5$, whose corrected count we have only at $10^6$, they are the same at $10^5$. For $\alpha = 5$ the count covers the starts that reached a cycle; the other $10.5\%$ escaped past coordinates $2^{100}$ (Section 5.5). Two of the cycles are long: $\alpha = 5-2i$ has one of $70$ odd steps through $15+16i$, and $\alpha = 6+5i$ one of $130$ odd steps through $164+199i$. At level $3$ we also tried the non-minimal digit sets made of the unit multiples of $2+i$, $2-i$ or $3$; each gives between $5$ and $14$ cycles.

*Proposition 5.* Among the symmetric, minimal, Collatz-like rules with $k \le 5$, every rule other than $H$ and its mirror image has at least two cycles up to units.

*Proof.* The table exhibits two cycles for each, and each was verified in exact arithmetic. $\square$

What is not proven is that $H$ has only one cycle. That is Conjecture 1.

### 5.3 Level $6$

At level $6$ there are $28$ multipliers and four minimal digit sets, so $112$ rules, $56$ up to conjugation; no rule is its own mirror image. Of the $112$, $22$ have a single cycle up to units for all starts with $N(z) \le 10^6$, with $\alpha = 7 \pm 4i$, $8 \pm 3i$, $9 \pm 2i$, $10 \pm i$, $10 \pm 3i$ and $11 \pm 2i$. Four of these have $\alpha = 11 \pm 2i$, of norm $125$ and drift $\ln(125/128)$ per odd step, and their excursions are large enough to need special handling (Section 7).

The $45$ rules up to conjugation with more than one cycle have these counts: $19$ have $2$ cycles, $4$ have $3$, $3$ have $4$, $5$ have $5$, one each has $6$ and $7$, $4$ have $8$, $2$ have $9$ and $2$ have $10$. The other four are the rules with $\alpha = 9$, with $23$ and $25$ cycles, and with $\alpha = 11$, with $560$ and $568$ cycles at $N(z) \le 10^6$ against $181$ and $211$ at $10^5$. For the rule with $568$, a further search over cycles whose least element has norm up to $3 \cdot 10^6$ raises the count to at least $639$, and the same holds for its mirror image (Section 5.6). All four have orbits that appear to diverge. For six of the $45$, those with $\alpha = 10 \pm 5i$ and the two with $\alpha = 11 \pm 2i$ that have two cycles, some starts reached the cap of $50000$ steps, so their counts are lower bounds; this does not affect the count of $22$. Apart from $\alpha = 11$, every count is the same at $10^5$ and $10^6$.

### 5.4 Level $7$

The window $(128, 256)$ holds $18$ norms and $46$ multipliers up to units. The $64$ odd classes modulo $(1+i)^7$ form $16$ orbits under the units, the minimal digit set is unique and closed under conjugation (Section 4.4), and so there are $46$ rules, $24$ up to conjugation, with $\alpha = 13$ and $\alpha = 15$ their own mirror images. Every start with $N(z) \le 10^6$ reached a cycle in every rule, with no step-cap hit and no escape.

Eight rules have a single cycle up to units: $\alpha = 11 \pm 4i$, $12 \pm 3i$, $11 \pm 8i$ and $12 \pm 7i$, of norms $137$, $153$, $185$ and $193$. In each the cycle is the fixed class of $1$. For one rule of each mirror pair the check of Section 7 extends this to all $N(z) \le 10^9$. Multipliers of the same norm can behave differently: $9 \pm 8i$ and $12 \pm i$, both of norm $145$, have $16$ and $2$ cycles.

The other $20$ rules up to conjugation have: $2$ cycles for $12 \pm i$; $3$ for nine rules; $4$ for $13 \pm 2i$; $5$ for $11 \pm 6i$ and $14 \pm i$; $8$ for $12 \pm 5i$; $11$ for $10 \pm 7i$; $16$ for $9 \pm 8i$; $33$ for $15 \pm 2i$; $74$ for $15$; $166$ for $13$; and $766$ for $11 \pm 10i$. Conjugate rules have the same count. The counts up to $16$ are for starts with $N(z) \le 10^6$. The last four are the complete counts of cycles with least norm at most $10^9$ from Section 5.6; at $10^6$ the sweep had found $33$, $72$, $166$ and $545$. Long cycles include a mirror pair of $537$ odd steps for $\alpha = 15$, through $4081+4664i$ and $4664+4081i$, and one of $21$ odd steps through $11+4i$ for $\alpha = 14+i$.

### 5.5 Divergence

Orbits that appear to diverge occur only for $\alpha = 5$ at level $4$ and $\alpha = 9, 11$ at level $6$, all of them real multipliers. No rule with a non-real multiplier showed such orbits.

For $\alpha = 5$, of about $1500$ random starts with norm up to $1.8 \cdot 10^7$, $176$ gave orbits that appear to diverge, and all of them settled onto the lines $x - y = \pm 1$ or $x + y = \pm 1$. At level $6$, of about $150$ random starts with $|x|, |y| \le 1000$ per digit set, $13$ to $29$ escaped past norm $2^{120}$ for $\alpha = 9$ and $55$ to $69$ for $\alpha = 11$, and every escaped orbit ended on one of the eight lines

$$L: \quad x = \pm 1, \quad y = \pm 1, \quad x - y = \pm 1, \quad x + y = \pm 1,$$

most of them on the diagonal ones. This is the analogue on lines of Kionke's divergent cones [8]. Matthews [4, Example 10.1] reports a similar picture for an extension of $3x+1$ to $\mathbb{Z}[\sqrt 2]$, whose multiplier $3$ is real: finitely many cycles found, three one-dimensional invariant sets, and apparently divergent orbits.

The set $L$ is invariant for every real multiplier we tested, at odd levels as well as even ones: $\alpha = 3$ at level $3$, $5$ at level $4$ with both minimal digit sets, $7$ at level $5$, $9$ and $11$ at level $6$ with all four, and $13$ and $15$ at level $7$. A point of $L$ has the form $u + tw$ with $u$ a unit, $w$ an associate of $1$ or $1+i$, and $t$ an integer. Since $\alpha$ is real, $\alpha z + d = \alpha t w + (\alpha u + d)$, and the first term lies on the line $\mathbb{R}w$, so the coordinate of $\alpha z + d$ transverse to $w$ is that of $\alpha u + d$. The digit depends only on $t$ modulo a power of $2$, so whether $S(z)$ lies in $L$, and with which valuation, depends only on $t$ modulo a fixed power of $2$. For each of these rules every $t$ in a window of $2^{16}$ consecutive integers starting at $10^{12}$ (and $2^{18}$ as a second check) was tested on two of the lines, the other six being their images under units. Every point mapped into $L$, with valuation $k$, $k+1$ or $k+2$. Since the valuations are bounded on every residue class of $t$, the window covers a full period, so the invariance is checked over a full residue period. An odd step with odd valuation turns a line of axis type, $x = \pm 1$ or $y = \pm 1$, into one of diagonal type, and back, which is why the earlier count of $876$ of $1000$ points of the diagonal lines mapping back onto them, for $\alpha = 5$, rises to all of them once the axis lines are included.

On $L$ the rule acts as a one-dimensional map, and $\log_2 |z|$ changes by $\frac12(\log_2 N(\alpha) - v)$ per odd step. Measured over a full period, the mean valuation and the transitions between the two types of line define a two-state chain, and its stationary distribution gives a mean drift along $L$:

| rule | mean $v$ | $\log_2 N(\alpha)$ | drift along $L$ (bits of $\vert z \vert$ per odd step) | observed |
|---|---|---|---|---|
| $3$, level 3 | 3.600 | 3.170 | $-0.215$ | converges |
| $5$, level 4 | 4.400 | 4.644 | $+0.122$ | diverges |
| $7$, level 5 | 5.778 | 5.615 | $-0.082$ | converges |
| $9$, level 6 | 6.222 | 6.340 | $+0.059$ | diverges |
| $11$, level 6 | 6.545 | 6.919 | $+0.187$ | diverges |
| $13$, level 7 | 7.684 | 7.401 | $-0.142$ | converges |
| $15$, level 7 | 7.882 | 7.814 | $-0.034$ | converges |

The values do not depend on the digit set. The sign of the drift matches the observed behaviour in every case. For $\alpha = 15$ a uniform average over $L$ would give $+0.021$, but the chain spends only $0.47$ of its time on lines of axis type, where valuations are larger, and the drift is negative. Exact iteration agrees: $300$ starts on $L$ for $\alpha = 15$, with $t$ up to $2 \cdot 10^9$, all returned to cycles, and $200$ starts on $L$ for $\alpha = 9$ all escaped. The drift is a heuristic. It assumes that the residues of $t$ are equidistributed along an orbit on $L$, as the drift of $3x+1$ assumes for residues modulo powers of $2$, and none of the divergence is proven.

The lines carry the divergence and not the cycles. For $\alpha = 11$ at level $6$, none of the cycles of $37$ odd steps that make up most of its count has an element on $L$.

### 5.6 Families of cycles

Five rules have many cycles: at level $7$, $\alpha = 13$, $15$, $11 \pm 10i$ and $15 \pm 2i$, and at level $6$, $\alpha = 9$ and $11$. A study of their cycles found no infinite family. No affine map relates two cycles except the mirror symmetry, the cycles are spread over all angles and do not lie near a line, and they share no parity vector beyond the fact that most valuations equal $k$. What organizes them is arithmetic.

Let a cycle up to units have $n$ odd steps $z_{j+1} = (\alpha z_j + d_j)/(1+i)^{v_j}$, total exponent $V = v_0 + \cdots + v_{n-1}$, and unit $\varepsilon$ with $S^n(z_0) = \varepsilon z_0$. With $s_j = v_j + \cdots + v_{n-1}$,

$$z_0 \left(\varepsilon (1+i)^V - \alpha^n\right) = \sum_{j=0}^{n-1} d_j\, \alpha^{n-1-j} (1+i)^{V - s_j}.$$

Put $r = |\alpha|^n / 2^{V/2}$ and $\text{rel} = |1 - \alpha^n/(\varepsilon(1+i)^V)|$. The right-hand side is of size about $2^{V/2}$ times a bounded factor, so $|z_0|$ is of order $1/\text{rel}$, and large cycles need $\alpha^n$ to be close to $\varepsilon (1+i)^V$. For real $\alpha$ this is the single condition that $V/(2n)$ approximate $\log_2 \alpha$; for non-real $\alpha$ the modulus and the angle must both be close. The cycles fall into classes $(n, V, \varepsilon)$, and the large classes sit at the near-coincidences:

| rule | main class $(n, V)$ | near-coincidence | rel | cycles in class | heuristic $E$ |
|---|---|---|---|---|---|
| $13$, level 7 | $(10, 74)$ | $13^{10} = 1.00305 \cdot 2^{37}$ | $3.05 \cdot 10^{-3}$ | 160 | 117 |
| $15$, level 7 | $(43, 336)$ | $15^{43} = 0.9974 \cdot 2^{168}$ | $2.56 \cdot 10^{-3}$ | 32 | 29.3 |
| $11 + 10i$, level 7 | $(33, 257)$ | $(11+10i)^{33}$ near a unit times $(1+i)^{257}$ | $6.17 \cdot 10^{-4}$ | 728 | 764 |
| $15 + 2i$, level 7 | $(6, 47)$ | $(15+2i)^6$ near $i(1+i)^{47}$ | $1.58 \cdot 10^{-2}$ | 28 | 20.5 |
| $11$, level 6 | $(37, 256)$ | $11^{37} = 0.99929 \cdot 2^{128}$ | $7.1 \cdot 10^{-4}$ | 498 or 481 at $10^6$ | 613 |
| $9$, level 6 | $(6, 38)$ | $9^6 = 1.0136 \cdot 2^{19}$ | $1.36 \cdot 10^{-2}$ | 15 or 17 | 18.4 |

For $11 + 10i$ both conditions hold at $n = 33$ to about $6 \cdot 10^{-4}$, which is why this rule has the most cycles. Smaller classes sit at multiples and sums of the resonant lengths, such as $66$ and $132$ for $11+10i$ and $75 = 43 + 32$ for $15$. Two classes are structural: for $\alpha = 15$ at level $7$ every digit $d$ gives a fixed point, since $-15(-d) + d = 16d = (1+i)^8 d$ up to a unit, and the same holds for $\alpha = 9$ at level $6$, with $16$ and $8$ fixed points.

The column $E$ is a heuristic count. The odd $(1+i)$-adic integers whose first $n$ valuations are $v_0, \ldots, v_{n-1}$ form $2^{(k-1)n}$ classes, on each of which $S^n$ is affine and onto, so each contains exactly one point with $S^n(z) = \varepsilon z$. That point is a Gaussian integer exactly when $\varepsilon(1+i)^V - \alpha^n$ divides an explicit numerator, and if this has probability $1/N(\varepsilon(1+i)^V - \alpha^n)$, the expected number of cycles up to units in the class is

$$E(n, V, \varepsilon) = \frac{2^{(k-1)n} \binom{V - kn + n - 1}{n - 1}}{4n\, N\!\left(\varepsilon(1+i)^V - \alpha^n\right)}.$$

It matches the main classes to within a factor of about $1.4$. Summed over all $n$ it converges, because $V/n$ must stay near $\log_2 N(\alpha)$, below the mean valuation $k+1$, and such valuation vectors become exponentially rare. The totals, $129$ for $13$, $70$ for $15$, $30.5$ for $15 + 2i$, $825$ for $11 + 10i$, $31$ for $9$ and $721$ for $11$, are close to the counts found, and the heuristic predicts finitely many cycles for each rule.

*Lemma 1.* Let a cycle have data $(n, V, \varepsilon)$, let $g_j = \ln|\alpha| - \frac{v_j}{2}\ln 2$, so that $\ln r = \sum_j g_j$, and let $c = \max(0, \ln r)$. Put $G_m = \min\left(m(\ln|\alpha| - \frac{k}{2}\ln 2),\, c\right)$ for $m < n$ and $G_n = \ln r$. Then some element $z$ of the cycle satisfies

$$|z| \le \frac{d_{\max}}{|\alpha|\, \text{rel}} \sum_{m=1}^{n} e^{G_m},$$

where $d_{\max}$ is the largest $|d|$ for $d \in D$.

*Proof.* By the cycle equation, for the cycle started at any of its elements, $|z_0|\, \text{rel} \le |\alpha|^{-1} \sum_{m=1}^{n} |d_{n-m}|\, e^{G'_m}$, where $G'_m$ is the sum of the last $m$ of the $g_j$. Start the cycle at an index where the prefix sums of the $g_j$ are least. Then every suffix sum is either the total minus a nonnegative amount or a sum that wraps around and is at most $0$, so $G'_m \le c$. Since every $v_j \ge k$, also $G'_m \le m(\ln|\alpha| - \frac{k}{2}\ln 2)$, and $G'_n = \ln r$. $\square$

If $|V - n\log_2 N(\alpha)| \ge 3$ then $r \ge 2^{3/2}$ or $r \le 2^{-3/2}$, and since $\text{rel} \ge |1 - r|$, the bound is at most $1.55\, d_{\max} n/|\alpha|$. So for each $n$ only six values of $V$ and four units need to be evaluated. The full classification by `gcz` finds every cycle with an element of norm at most its bound $B$, and `gstop` over $(B, 10^9]$ finds every cycle whose least element has norm in that range. Taking $B$ at least the bound of Lemma 1 for every $n$ up to a length $n_{\max}$ gives:

*Theorem 1 (computer-assisted).* For the level-$7$ rules with the minimal digit set:

1. $\alpha = 13$ has exactly $166$ cycles up to units with least norm at most $10^9$, and these include every cycle with at most $216$ odd steps; exactly $160$ have $10$ odd steps.
2. $\alpha = 15$ has exactly $74$, including every cycle with at most $246$ odd steps; exactly $32$ have $43$ odd steps.
3. $\alpha = 11 + 10i$, and by conjugation $11 - 10i$, has exactly $766$, including every cycle with at most $2000$ odd steps; exactly $728$ have $33$ odd steps.
4. $\alpha = 15 + 2i$, and by conjugation $15 - 2i$, has exactly $33$, including every cycle with at most $1498$ odd steps; exactly $28$ have $6$ odd steps.

The proof consists of Lemma 1, runs of `gcz` to $N(z) \le 1.3 \cdot 10^7$, $1.65 \cdot 10^7$, $10^7$ and $2.85 \cdot 10^7$, and runs of `gstop` from those bounds to $10^9$. In the run for $\alpha = 15$, $8$ starts reached the step cap; iterated in exact arithmetic, each reaches the $537$-step pair without first falling below its own norm. The theorem relies on the correctness of the two C programs and on double-precision evaluation of the bound, which is used with a margin of at least $1.4$ wherever it decides coverage; rel was confirmed in exact integer arithmetic for the main classes. Every cycle in the lists was checked in exact Gaussian arithmetic: each digit lies in $D$ and is the one the rule picks, each element is odd, the cycle equation holds, and the elements are pairwise non-associate.

For $\alpha = 11$ at level $6$ there is no complete list. A pass of `gstop` over least norms in $(10^6, 3 \cdot 10^6]$ for one digit set found $172$ cycles, $101$ of them already known, so that rule has at least $639$ cycles. The bound of Lemma 1 for $n = 37$ is about $5.6 \cdot 10^8$, out of reach of `gstop` for this rule, whose orbits often fall into cycles lying entirely above their starting norm and run to the step cap.

What is proven is Lemma 1. Theorem 1 is computer-assisted, resting on the programs as stated. The heuristic count, the expectation of finitely many cycles, and the absence of further cycles longer than the stated lengths with least norm above $10^9$ are heuristic. Whether any of these rules has finitely many cycles is open. Proving finiteness would need an effective lower bound for $|\varepsilon(1+i)^V - \alpha^n|/2^{V/2}$ for all large $n$, a linear form in two logarithms for real $\alpha$, or in a logarithm and an angle for non-real $\alpha$. With a bound of Baker type, Lemma 1 would turn each check into a finite one, as Simons and de Weger [21] do for $3x+1$ after Steiner [20]. We have not attempted this.

## 6. The rule H

### 6.1 Definition

$H$ has $\alpha = 5+2i$, $N(\alpha) = 29$ and $k = 4$, so $16 < 29 < 32$. Since $(1+i)^4 = -4$, classes modulo $(1+i)^4$ are classes modulo $4$. There are $8$ odd classes in two orbits under the units, the classes of the units and those of the unit multiples of $1+2i$, and the digit set is

$$D = \{\pm 1,\ \pm i,\ \pm(2+i),\ \pm(1-2i)\},$$

the units together with the unit multiples of $2+i$. The digit $d(z)$ is the element of $D$ that makes $(5+2i)z + d(z)$ divisible by $4$:

| $z \bmod 4$ | $1$ | $i$ | $-1$ | $-i$ | $1+2i$ | $2+i$ | $-1-2i$ | $2-i$ |
|---|---|---|---|---|---|---|---|---|
| $d(z)$ | $-1+2i$ | $-2-i$ | $1-2i$ | $2+i$ | $-1$ | $-i$ | $1$ | $i$ |

The table follows from one congruence. Modulo $4$, $\alpha \equiv 1+2i$, and $(1+2i)^2 = -3+4i \equiv 1$. If $z \equiv u$ for a unit $u$, the digit must be $\equiv -(1+2i)u$; no unit lies in that class, and its elements of least norm are unit multiples of $2+i$. If $z \equiv (1+2i)u$, the digit must be $\equiv -(1+2i)^2 u \equiv -u$, the unit $-u$. Multiplication by $\alpha$ swaps the two orbits of classes, and the digits alternate between units and associates of $2+i$. For example, $(5+2i) \cdot 1 + (-1+2i) = 4+4i = -(1+i)^5$, so $S(1) = -1$, and by symmetry $S(-1) = 1$. The other minimal digit set gives the mirror image of the rule with $\alpha = 5-2i$.

Where $3x+1$ has one digit, $H$ has eight, because the window forces level $4$, and there are $2^{k-1} = 8$ odd classes modulo $(1+i)^4$.

### 6.2 Orbits

Orbits of $H$ rise and fall as orbits of $3x+1$ do. Under $S$ the orbit of $3$ is

$$3 \to -4-i \to 4+3i \to -3+2i \to 2-3i \to -4+3i \to -2-i \to -i,$$

with norms $9, 17, 25, 13, 13, 25, 5, 1$ and $4, 4, 6, 5, 4, 7, 7$ factors of $1+i$ removed at the successive steps. Longer orbits are common: $6827+4464i$ takes $2042$ odd steps to reach a unit.

### 6.3 The backward form

Lagarias [14] restates $3x+1$ in terms of the inverse maps $2x$ and $(2x-1)/3$: once an iterate of these maps is not an integer it never is again, so the integers in the orbit of $4$ under the semigroup they generate form the backward tree, and the conjecture says that this set is every positive integer except $1$ and $2$. The same holds for $H$, in the form of the rule that performs all $k$ divisions at once, $z \mapsto (\alpha z + d(z))/(1+i)^4$ on odd $z$ and $z \mapsto z/(1+i)$ on even $z$. Its orbits are subsequences of those of $H$, and the intermediate values $(\alpha z + d)/(1+i)^j$ with $j < 4$ are even, so the two rules reach a unit together. Its inverse branches are

$$w \mapsto (1+i)w \quad \text{and} \quad w \mapsto \frac{-4w - d}{5+2i}, \quad d \in D.$$

If $-4w - d$ is divisible by $5+2i$, its quotient $z$ satisfies $\alpha z + d = (1+i)^4 w$, which forces $z$ to be odd and $d$ to be the digit of $z$. If $w$ has $(5+2i)^j$ with $j \ge 1$ in its denominator, so does $-4w - d$, because $4$ and $d$ are prime to $5+2i$, and the second branch raises the exponent to $j+1$; the first leaves it unchanged. So the Gaussian integers in the semigroup orbit are exactly the backward tree, and Conjecture 1 says:

*Conjecture 1, backward form.* The Gaussian integers in the orbit of $\{1, i\}$ under the semigroup generated by $w \mapsto (1+i)w$ and $w \mapsto (-4w-d)/(5+2i)$, $d \in D$, are all of $\mathbb{Z}[i] \setminus \{0\}$.

The starting set $\{1, i\}$ suffices because $-1$ and $-i$ are the images of $1$ and $i$ under backward steps. As for $3x+1$, the orbit is not discrete, since the second branch multiplies the norm by $16/29 < 1$, so density arguments of Erdős's kind for semigroups of expanding affine maps [14] do not apply.

### 6.4 Drift and excursions

Every odd step of $H$ gains at least four factors of $1+i$, which divide the norm by $16$, against a factor of $29$; on average it gains five, which divide by $32$. So the norm changes on average by a factor $29/32$ per odd step, $\log_2 29 - 5 = -0.14$ bits. For $3x+1$, measured on $x^2$ so that the two compare, the factor is $(3/4)^2 = 9/16$, or $-0.83$ bits. $H$ contracts about six times more slowly, so its orbits can climb far before they fall.

A random-walk model makes this quantitative. Treat the excess divisions $v - k$ at successive odd steps as independent geometric variables. Then $\log_2 N$ performs a random walk with drift $-0.142$ bits and variance $2$ per odd step. Simulating $4 \cdot 10^6$ such walks gives a mean of $53.8$ steps of $T$ until the walk first falls below its start; the check of Section 7 measured between $53.82$ and $53.93$ in each of its $30$ chunks. For the largest excursion, the probability that the walk climbs $h$ bits above its start before stopping is about $C e^{-\theta h}$, where $\theta$ solves $\mathbb{E}[e^{\theta X}] = 1$ for the change $X$ per odd step. Here $\theta = 0.165$ per bit, and simulation gives $C \approx 0.36$, with the peak taken just after each multiplication. Over $n$ independent starts the record then has median about $(\ln n + \ln C - \ln \ln 2)/\theta$, which for the $1.18 \cdot 10^9$ starts of the check is $122.4$ bits. The observed record is $122.9$ bits, at $17467+5226i$. The agreement is closer than the model justifies, because the starts are not independent: the records in $14$ of the $30$ chunks come from orbits that pass through one point of norm about $2^{151.2}$ (checked directly for five of them). The mean stopping time is the better test of the model.

The level-$7$ single-cycle rules are the contrast. Their drift is between $-0.90$ bits (norm $137$) and $-0.41$ bits (norm $193$) per odd step, against $-0.14$ for $H$. Over the same range, all starts with $N(z) \le 10^9$, their largest excursions are $2^{9.7}$, $2^{14.8}$, $2^{29.2}$ and $2^{37.4}$ in norm, in order of increasing norm, and their longest stopping times $284$ to $2243$ steps of $T$. For $H$ the largest excursion over that range is $2^{122.9}$ and the longest stopping time $12226$. Up to $N(z) \le 10^6$ the level-$7$ excursions stay below $2^{25}$. By the model, the weaker the drift the heavier the tail of the excursions, and among the symmetric, minimal rules with a single cycle known to us, $H$ has the weakest drift apart from the four level-$6$ rules with $\alpha = 11 \pm 2i$.

## 7. Computational verification

The program `gcz.c` classifies every $z$ with $N(z) \le B$. It stores one entry per lattice point, follows each orbit until it reaches a point already classified, closes a cycle (found with Brent's method), escapes past coordinates $2^{100}$ or reaches a step cap, and fills in the result along the path. Coordinates are 128-bit integers. A rule is given as a table over the residues of $(a \bmod M, b \bmod M)$, with $M = 4$, $8$ or $16$; modulo $16$ fixes the class modulo $(1+i)^7$.

For symmetric rules, `gstop.c` checks a larger range by the drop-below-start method. For each odd $z = a+bi$ with $a > 0$, $b \ge 0$ and $N_0 < N(z) \le B$, it iterates $T$ until the norm falls below $N(z)$, and it reports any orbit that returns to an associate of its start, reaches the step cap, or escapes past coordinates $2^{110}$. If every such start falls below its own norm, then by induction on the norm every $z$ with $N(z) \le B$, of any parity and any associate, reaches a norm at most $N_0$ (Theorem `drop_below_start_sound`, Section 8), and so ends in one of the cycles that `gcz` found below $N_0$. Norms are compared exactly in 128-bit arithmetic while they are below $10^{36}$, that is while coordinates are below about $2^{60}$; above that the iteration continues without comparing, so the method cannot report a false drop.

For $H$, `gcz` classified every $z$ with $N(z) \le 10^7$ and found only the cycles through $\pm 1$ and $\pm i$. Then `gstop` covered $10^7 < N(z) \le 3 \cdot 10^9$ in $30$ chunks, $1.18 \cdot 10^9$ starts, with no failure and no other cycle, in about $15$ minutes on one core. The largest coordinates met were below $2^{80}$. Over that range the longest stopping time is $13913$ steps of $T$, at $6742+34011i$, the mean is $53.9$, and the largest excursion, $\log_2$ of the peak norm before stopping divided by the starting norm, is $122.9$, at $17467+5226i$. The rule file read by `gstop` agrees entry for entry with the digit table proved correct in Lean.

The Python module `exact.py` iterates the same rules in unbounded integer arithmetic and chooses each digit from the congruence. It re-derives every cycle reported here and agrees with `gcz` and `gstop` on the extreme orbits. The sweeps behind the census are `sweep_eq2.py` (symmetric rules at levels $3$ to $7$, at $N(z) \le 10^5$ and $10^6$) and `sweep_gen.py` (the non-symmetric rules of Section 3.2).

Other runs of `gstop` cover $G_3$ and $G_{3+2i}$ to $N(z) \le 10^9$, $3.93 \cdot 10^8$ starts each, with no failure and no new cycle; and the level-$7$ single-cycle rules with $\alpha = -11+4i$, $-12+3i$, $-11+8i$ and $-12+7i$, associates of one rule from each mirror pair, over $10^6 < N(z) \le 10^9$, $392306389$ starts each, with no failure and no cycle, after all eight rules had been checked from norm $1$ to $10^6$. For the four level-$6$ rules with $\alpha = 11 \pm 2i$, whose excursions pass the escape bound of `gcz`, `gstop` was run from norm $1$ to $10^6$ with escape at $2^{110}$, and the $26$ starts that escaped even that bound were iterated in unbounded arithmetic until their norm fell below the starting norm, which took up to $138239$ steps of $T$ with excursions up to about $2^{300}$.

## 8. Formal verification

The Lean 4 development in `collatz/research/lean/` proves the following, with Gaussian integers as pairs of integers and $\pi = 1+i$.

| statement | Lean theorems |
|---|---|
| Proposition 2: for $k \le 2$, a digit function that depends only on the class modulo $\pi^k$ and takes odd values has $d(-z) \ne -d(z)$ for some odd $z$ | `prop1`, `pi_sq_dvd_two_mul` |
| Proposition 4: no Gaussian integer has norm $3$ | `prop3_int`, `prop3` |
| Proposition 1(2): if $\pi^k$ divides $3n+1$ for every odd natural $n$, then $k \le 2$ | `prop5_2`, `prop5_2'` |
| Soundness of the drop-below-start check, for any $T$ that divides by $\pi$ on even inputs and commutes with units | `drop_below_start_sound` |
| $\pi^4 = -4$; the digit table of $H$ is correct for every odd $z$, and its representatives are exactly the eight odd classes modulo $4$ | `pi_pow_four`, `four_dvd_class`, `tableH_reps`, `tableH_distinct`, `tableH_covers`, `digitH_correct` |
| From $1$, one odd step to $4+4i$ and five divisions through even values give $-1$, and from $-1$ back to $1$ | `H_cycle` |
| $H$ divides by $\pi$ on even inputs and commutes with units | `H_div`, `digitH_unit`, `H_unit` |
| The check applied to $H$: if every odd $z$ with $a > 0$, $b \ge 0$, $N_0 < N(z) \le B$ has an iterate of smaller norm, then every $z$ with $N(z) \le B$ has an iterate of norm at most $N_0$ | `H_sound` |

The names `prop1`, `prop3` and `prop5_2` follow the numbering of the first draft. In Proposition 4 the step from the window $2 < N(\alpha) < 4$ to $N(\alpha) = 3$, and in Proposition 1(2) the step from $k \le 2$ to $9 > 2^{k+1}$, are not formalized. Propositions 1(1), 3 and 5 and the census are not formalized.

The development uses core Lean 4 (toolchain `v4.35.0`) and no Mathlib or other library. It contains no `sorry` or `admit` and no `native_decide` or `bv_decide`, so every `decide` proof is checked by the kernel. The tactics are `omega`, `decide`, `simp`, `grind`, `induction` and `rcases`. The file `CheckAxioms.lean` prints the axioms of every main theorem: each depends on at most `propext`, `Classical.choice` and `Quot.sound`, and `pi_pow_four` and `digitH_unit_table` on none. We rebuilt the development and reran the axiom check for this draft.

What is trusted is the computation. `H_sound` turns the bound $3 \cdot 10^9$ into a theorem given two facts that Lean does not check: that the `gstop` run established its hypothesis with $N_0 = 10^7$ and $B = 3 \cdot 10^9$, and that the `gcz` run found every cycle reached from $N(z) \le 10^7$. Both rest on the C programs, their 128-bit arithmetic, the compiler, and the agreement of the rule file with `digitH`.

## 9. Open questions

1. *A second cycle or a divergent orbit for $H$.* A cycle of $H$ up to units with $n$ odd steps $z_j \mapsto z_{j+1}$ and total division exponent $V$ satisfies $\prod_j (\alpha + d_j/z_j) = \varepsilon (1+i)^V$ for a unit $\varepsilon$. When the $z_j$ are large this forces $2^V$ to be close to $29^n$ and the argument of $(5+2i)^n$ to be close to $V\pi/4$ modulo $\pi/2$. The second condition has no analogue for $3x+1$. Bounds of the Steiner and Simons–de Weger type [20, 21], which use linear forms in logarithms, might be adapted to both conditions together and exclude cycles with few odd steps. The quantity to bound from below is $|\varepsilon(1+i)^V - \alpha^n|$, the same one whose lower bound would turn the complete lists of Section 5.6 into finiteness theorems.
2. *The level-$6$ and level-$7$ single-cycle rules.* The same question for the $22$ rules at level $6$ and the $8$ at level $7$. The four with $\alpha = 11 \pm 2i$, with drift $\ln(125/128)$, have the heaviest excursions of any rule here.
3. *Higher levels and many-cycle rules.* Level $8$ has the window $(256, 512)$. Whether single-cycle rules stay a minority there, as at levels $6$ and $7$, is open, as is whether the many-cycle rules of Section 5.6 have finitely many cycles.
4. *Divergence for real $\alpha$.* The eight lines of Section 5.5 are invariant for each real rule tested, and on them the rule is a one-dimensional map. Proving that some orbit on them grows without bound for $\alpha = 5$, $9$ or $11$ would replace the equidistribution heuristic behind the sign of the drift with a proof.
5. *A density theorem.* Terras [22] and Everett [23] showed that almost every positive integer has a finite stopping time under $3x+1$, because the parity pattern of the first $n$ steps depends only on $x \bmod 2^n$. The analogous pattern for these rules depends only on $z$ modulo a power of $1+i$, and we expect the same argument to give finite stopping time on a set of density one whenever $N(\alpha) < 2^{k+1}$; this has not been worked out. It would not rule out divergence, since Kionke's examples [8] show that density-one stopping and positive-density divergence can coexist. Nichols [24] proved a theorem of Terras type in function rings of curves over $\mathbb{F}_2$ [unverified: only the abstract seen], and Siegel and O'Dwyer [15] prove almost-bounded orbits on a set of logarithmic density one for Hydra maps on rings of integers under a dimension constraint. Whether $H$ or an iterate of it meets their hypotheses is open; a direct encoding modulo $(1+i)^4$ appears to fail their constraint [unverified: a rough estimate].
6. *Generalized rules that restrict to $3x+1$.* Proposition 1 rules out a single multiplier $3$ at a single level. Rules of Matthews type [4], in which the multiplier or the level depends on the class, remain. Such a rule would need level at most $2$ on the class of $3 \bmod 4$ and could compensate with higher levels elsewhere. We have not tested any. By Conway's theorem [7] such rules have to be studied one at a time, and if one were found with a single cycle, proving that would include proving the Collatz conjecture.
7. *The $2$-adic extension.* Since the digit of $H$ depends only on $z \bmod 4$, $H$ extends continuously to $\mathbb{Z}_2[i]$. For $3x+1$ the $2$-adic extension preserves Haar measure and is conjugate to the shift [1, 25]. Whether the analogues hold for $H$ is open.
8. *Larger bounds.* At about $1.2 \cdot 10^6$ starts per second per core, checking $H$ to $10^{11}$ needs about nine core-hours.

## 10. Code and data availability

Everything is in `collatz/research/`. The C programs are `gcz.c` (full classification) and `gstop.c` (drop-below-start check), with the rule files `alpha5p2i_k4.rule` ($H$), `G3_k3.rule`, `G3p2i_k3.rule` and `k7_a-11_4.rule`, `k7_a-12_3.rule`, `k7_a-11_8.rule`, `k7_a-12_7.rule`. The Python files are `rules.py` (rule tables and digit rules), `exact.py` (exact iteration and cycle checks), `sweep_eq.py` and `sweep_eq2.py` (symmetric sweeps), `sweep_gen.py` (non-symmetric sweeps), `joseph.py` (Joseph-type maps), `bigrun.py` and `run_stop.sh` (driving the large runs), `zsym.py` (the integer rules of Section 3.6), `below_window.py` (Section 3.8), and the scripts of the cycle-family study, `families.py`, `famtheory.py` (Lemma 1 and the heuristic count), `famcover.py`, `famheur.py`, `famstruct.py`, `famverify.py` (exact check of every cycle), `famlines.py`, `famline1d.py` (invariance of $L$ and the drift along it), `famlinerun.py` and `famescape.py`; the study itself is written up in `families.md`. Logs of the runs are in `logs/`, among them `alpha5p2i_k4.stop` (the $3 \cdot 10^9$ check of $H$, chunk by chunk), `G3_k3.stop`, `G3p2i_k3.stop`, `k345_1e6_rerun.txt`, `k6_1e6.txt`, `k6_1e6_alpha9_11_rerun.txt`, `k7_1e6.txt`, `k7_single_gstop_1e9.txt`, and the files `families_*` of the cycle-family study, among them `families_gstop_k7_summary_1e9.txt`, `families_cover.txt` and `families_line_drift.txt`. The Lean proofs are in `lean/`, built with `lake build`, with axioms printed by `lake env lean CheckAxioms.lean`.

## References

1. J. C. Lagarias, The $3x+1$ problem and its generalizations, *American Mathematical Monthly* 92 (1985), 3–23.
2. J. C. Lagarias, The $3x+1$ problem: an annotated bibliography (1963–1999), arXiv:math/0309224.
3. J. C. Lagarias, The $3x+1$ problem: an annotated bibliography, II (2000–2009), arXiv:math/0608208.
4. K. R. Matthews, Generalized $3x+1$ mappings: Markov chains and ergodic theory, in J. C. Lagarias (ed.), *The Ultimate Challenge: The $3x+1$ Problem*, American Mathematical Society, 2010, 79–103. Preprint: http://www.numbertheory.org/PDFS/matthews-final-revised.pdf.
5. M. C. Siegel, $(p,q)$-adic analysis and the Collatz conjecture, arXiv:2412.02902.
6. S. Kohl, RCWA: Residue-Class-Wise Affine Groups, GAP package; see also S. Kohl, *Restklassenweise affine Gruppen*, Dissertation, Universität Stuttgart, 2005.
7. J. H. Conway, Unpredictable iterations, in *Proceedings of the 1972 Number Theory Conference*, University of Colorado, Boulder, 1972, 49–52 [unverified: not seen; citation copied from [14]].
8. S. Kionke, A geometric approach to divergent points of higher dimensional Collatz mappings, arXiv:1511.05893 (2015); *Monatshefte für Mathematik* 182 (2017) [unverified: journal details from a secondary source].
9. J. A. Joseph, A chaotic extension of the $3x+1$ function to $\mathbb{Z}_2[i]$, *Fibonacci Quarterly* 36(4) (1998), 309–316.
10. G. I. Kucinski, Cycles for the $3x+1$ map on the Gaussian integers, preprint, University of Scranton, May 2000. https://monks.scranton.edu/files/pubs/kucinski.pdf
11. M. Chamberland, An update on the $3x+1$ problem. https://chamberland.math.grinnell.edu/papers/3x_survey_eng.pdf
12. A. Alvarado, The Collatz conjecture over the Gaussian integers, talk slides, University of Colima, 2 April 2025, on work of a 2017 REU at CSU Channel Islands. https://fefoaranda.com/wp-content/uploads/2025/04/colima-alvarado-2025.pdf [unverified: slides only].
13. D. A. Klarner and R. Rado, Arithmetic properties of certain recursively defined sets, *Pacific Journal of Mathematics* 53(2) (1974), 445–463 [unverified: not seen; citation copied from [14]].
14. J. C. Lagarias, Erdős, Klarner, and the $3x+1$ problem, *American Mathematical Monthly* 123(8) (2016), 753–776.
15. M. Siegel and R. O'Dwyer, Almost-boundedness of Collatz-type maps on number rings via Archimedean methods, arXiv:2609.33018 (2026).
16. A. Garg, Collatz-type dynamics on Gaussian integers: fractal stability islands, long cycles, and dyadic rational structure, Zenodo, doi:10.5281/zenodo.19078383 (March 2026) [unverified: seen only through a summary].
17. Aliyev, *Notes on Number Theory and Discrete Mathematics* 31(3) (2025), 658–666.
18. K. R. Matthews and A. M. Watts, A generalization of Hasse's generalization of the Syracuse algorithm, *Acta Arithmetica* 43 (1984), 167–175.
19. H. Möller, Über Hasses Verallgemeinerung des Syracuse-Algorithmus (Kakutanis Problem), *Acta Arithmetica* 34 (1978), 219–226.
20. R. P. Steiner, A theorem on the Syracuse problem, *Proceedings of the 7th Manitoba Conference on Numerical Mathematics* (1977), 553–559.
21. J. Simons and B. de Weger, Theoretical and computational bounds for $m$-cycles of the $3n+1$ problem, *Acta Arithmetica* 117 (2005), 51–70.
22. R. Terras, A stopping time problem on the positive integers, *Acta Arithmetica* 30 (1976), 241–252.
23. C. J. Everett, Iteration of the number-theoretic function $f(2n) = n$, $f(2n+1) = 3n+2$, *Advances in Mathematics* 25 (1977), 42–45.
24. D. Nichols, Analogues of the $3x+1$ problem in polynomial rings of characteristic $2$, *Experimental Mathematics* (2016), arXiv:1610.02545.
25. D. J. Bernstein, A non-iterative $2$-adic statement of the $3N+1$ conjecture, *Proceedings of the American Mathematical Society* 121 (1994), 405–408.

[unverified: references 4, 8 (arXiv version), 9, 10 and 11 were read through text extraction, and 14 from page images in full. Reference 1 is confirmed by the reference list of [14]. The titles, page ranges and initials in references 17 to 23 and 25 are given from memory, from the working notes or from other papers' bibliographies, and have not been checked against the publications.]
