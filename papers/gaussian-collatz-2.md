# Collatz maps on the Gaussian integers

*Second draft, 4 November 2026. Code, logs and Lean proofs: `research/collatz/`. Items marked [unverified] have not been checked against their source or by computation.*

## Abstract

We study maps on the Gaussian integers built like the $3x+1$ map, with the prime $1+i$ in the role of $2$. An element divisible by $1+i$ is divided by it; any other $z$ is sent to $\alpha z + d$, where the digit $d$ is chosen by the class of $z$ modulo $(1+i)^k$ so that the result is divisible by $(1+i)^k$. We ask three things of such a rule: that it commute with multiplication by units, so that it treats $z$ and $-z$ alike, which $3x+1$ does not; that its digits be as small as possible; and that it expand in the worst case and contract on average, which happens exactly when $2^k < N(\alpha) < 2^{k+1}$. No such rule exists at levels $k \le 2$. Ordered by the norm of $\alpha$, the first such rule with a single cycle up to units is $H$, with $\alpha = 5+2i$, $k = 4$ and digits $\pm 1, \pm i, \pm(2+i), \pm(1-2i)$. It is the only one at level $4$ up to conjugation; levels $3$ and $5$ have none, and levels $6$ and $7$ have $22$ of $112$ and $8$ of $46$. We conjecture that every orbit of $H$ on $\mathbb{Z}[i] \setminus \{0\}$ reaches $\pm 1$ or $\pm i$, and we have checked this for all starts of norm at most $3 \cdot 10^9$. The multiplier $3$ falls at level $3$, where its rule has five cycles, and no rule with $\alpha = 3$ both agrees with $3x+1$ on the positive integers and contracts on average. The elementary propositions, the soundness of the check, and the facts about $H$ that the check uses are proved in Lean 4.

## 1. Introduction

The $3x+1$ map sends an even integer $x$ to $x/2$ and an odd one to $3x+1$. The Collatz conjecture says that every positive integer reaches $1$ [1]. The map has an accelerated form on odd numbers, $x \mapsto (3x+1)/2^v$ with $2^v$ the largest power of $2$ dividing $3x+1$. Each odd step multiplies $x$ by about $3$ and is followed by at least one division by $2$, so in the worst case $x$ grows by a factor $3/2$. For a random odd $x$ the number of divisions has mean $2$, so on average $x$ shrinks by a factor $3/4$. The problem lives in this gap between expansion in the worst case and contraction on average.

The same map makes sense on all of $\mathbb{Z}$, and there it has more cycles. Besides the fixed point $0$ and the cycle $1 \to 4 \to 2$, it has the cycle $-1 \to -2$ of $2$ steps, one through $-5$ of $5$ steps and one through $-17$ of $18$ steps. None of these is the negative of $1 \to 4 \to 2$. The reason is that $3x+1$ does not commute with $-1$: $-(3x+1) = 3(-x) - 1$, so on the negative integers the map acts as $3x-1$ acts on the positive ones, and $3x-1$ has three cycles. The single cycle conjectured for $3x+1$ is a property of one half of $\mathbb{Z} \setminus \{0\}$.

A conjecture of Collatz type can fail in two ways: some orbit falls into a second cycle, or some orbit grows without bound. Its content is that there is a single cycle and that every orbit reaches it. The example of $\mathbb{Z}$ suggests two requirements for a rule that is meant to carry such a conjecture on a ring larger than $\mathbb{N}$. The rule should commute with the units of the ring, so that no half of the ring is singled out, and its cycles should be counted up to units.

This paper asks what the natural analogue of $3x+1$ on the Gaussian integers is. The prime $1+i$, of norm $2$, plays the part of $2$, and a rule multiplies by a Gaussian integer $\alpha$ and adds a digit from a fixed set. Section 3 states three criteria for such a rule (symmetry under units, minimal digits, and the window $2^k < N(\alpha) < 2^{k+1}$ in which it expands in the worst case and contracts on average) and orders the rules by the norm of $\alpha$. The first rule in this order with a single cycle up to units is

$$H(z) = \begin{cases} z/(1+i) & \text{if } (1+i) \mid z, \\ (5+2i)z + d(z) & \text{otherwise,} \end{cases}$$

where $d(z)$ is the element of $\{\pm 1, \pm i, \pm(2+i), \pm(1-2i)\}$ that makes $(5+2i)z + d(z)$ divisible by $4$.

*Conjecture 1.* Every orbit of $H$ on $\mathbb{Z}[i] \setminus \{0\}$ reaches $\pm 1$ or $\pm i$. Up to units, $H$ has a single cycle.

On $\mathbb{Z}[i]$ itself the cycles are the two through $\pm 1$ and through $\pm i$, each the image of the other under multiplication by $i$. We have checked Conjecture 1 for every start of norm at most $3 \cdot 10^9$ (Section 7). The check rests on a reduction to one starting point per class of associates, and that reduction, the elementary propositions of Sections 3 and 4, and the facts about $H$ the reduction needs are proved in Lean 4 without external libraries (Section 8).

Section 2 surveys prior work. Section 3 sets up the family and the criteria, and Section 4 proves the basic facts about them. Section 5 is a census of the rules at levels $3$ to $7$. Section 6 studies $H$, Section 7 the computation, Section 8 the formal proofs, and Section 9 lists open questions.

## 2. Prior work

Lagarias's annotated bibliographies [2, 3] record the generalizations of $3x+1$ to other rings. We read them in part [unverified: bibliography I was read up to about entry 100, and bibliography II in its second version in full and in its current version up to entry 89].

The family studied here is a case of several general frameworks. Matthews [4, Section 10] considers generalized $3x+1$ maps on the ring of integers $\mathcal{O}_K$ of a number field, taken modulo a non-unit $d$, and works examples on $\mathbb{Z}[\sqrt 2]$ and $\mathbb{Z}[\sqrt 3]$; our rules are maps of this kind on $\mathbb{Z}[i]$ with $d = (1+i)^k$, though $\mathbb{Z}[i]$ is not among his examples. As reported by Siegel [5], Leigh was the first to consider Collatz-type maps on rings of algebraic integers, and Matthews knew of no other work on them [unverified: Leigh's work not seen]. Kohl's residue-class-wise affine maps [6] include maps of $\mathbb{Z}^2$ whose pieces have $2 \times 2$ integer matrix multipliers and lattice moduli, and each step of our rules is such a map [unverified: taken from the package source; no worked $\mathbb{Z}^2$ example seen]. Siegel's Hydra maps [5] cover maps on rings of integers in general, without examples in $\mathbb{Z}[i]$. Conway [7] showed in 1972 that a generalization of $3x+1$ to residue-class-wise affine maps of $\mathbb{Z}$ is undecidable [unverified: known through [13]], so questions about such maps in general have no algorithmic answer and must be settled rule by rule.

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

The same criteria can be applied in $\mathbb{Z}$, with $2$ in place of $1+i$ and $-1$ as the only nontrivial unit. A rule at level $k$ has digits for the odd classes modulo $2^k$ and is Collatz-like when $2^k < m < 2^{k+1}$ for its multiplier $m$. The multiplier $3$ forces level $1$, where there is a single odd class and $x \equiv -x \pmod 2$, so $x$ and $-x$ receive the same digit and no rule commutes with $-1$. This is why $3x+1$ is not symmetric. Symmetric integer rules exist from level $2$ on, with multiplier $5$ or $7$ and digits $\pm 1$: the digit is $-1$ for $x \equiv 1 \pmod 4$ and $+1$ for $x \equiv 3$ when $m = 5$, and the reverse when $m = 7$. For this draft we followed every odd start with $|x| \le 10^5$ under both. The rule $5x \pm 1$ has two cycles up to sign, the fixed point $1$ and $7 \to 9 \to 11$. The rule $7x \pm 1$ has only the fixed point $1$ up to sign, and every odd $x$ with $3 \le x \le 10^7$ falls below $x$ in absolute value, so that holds for all $|x| \le 10^7$. We have not looked for these two rules in the literature. So in $\mathbb{Z}$ the first symmetric Collatz-like rule with a single cycle, in the order of Section 3.5, is $7x \pm 1$, at the lowest level that has symmetric rules. In $\mathbb{Z}[i]$ that level is $3$, where no rule has a single cycle, and the first is $H$ at level $4$. In both rings the first single-cycle rule has the largest multiplier at its level.

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

which is less than $|z|$ once $|z| > 1/(2^{3/2} - \sqrt 5) \approx 1.69$. Every odd non-unit has norm at least $5$, so every odd step from a non-unit lowers the norm, and every orbit reaches a unit. From $1$ the digit is $1$, and $(1+2i) + 1 = 2+2i = -i(1+i)^3$, so $S(1) = -i$. By symmetry $S(-i) = -1$, $S(-1) = i$ and $S(i) = 1$, so the rule has exactly one cycle on $\mathbb{Z}[i] \setminus \{0\}$, through all four units. We confirmed this with the exact-arithmetic code of Section 7 for every odd start with $N(z) \le 10^5$, for $\alpha = 1+2i$, $2+i$, $1-2i$ and $2-i$: every odd step lowered the norm, every orbit reached a unit within $20$ odd steps, and the only cycle up to units is the class of $1$. Such a rule is the Gaussian counterpart of the $x+1$ problem, which Lagarias [14] calls the solved cousin of $3x+1$.

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
