# Collatz maps on the Gaussian integers

*Draft research note, 4 November 2026. Code and outputs: `research/collatz/`. Items marked [unverified] have not been checked against their source or by computation.*

## Abstract

We study maps on the Gaussian integers built like the $3x+1$ map, with the prime $1+i$ in the role of $2$. A number is even if $1+i$ divides it. An even $z$ is divided by $1+i$; an odd $z$ is sent to $\alpha z + d$, where the added term $d$ is chosen by the class of $z$ modulo $(1+i)^k$ so that the result is divisible by $(1+i)^k$. Such a map expands in the worst case and contracts on average, as $3x+1$ does, exactly when $2^k < N(\alpha) < 2^{k+1}$. For $k = 1$ this window holds no norm at all, so there is no Gaussian rule with a single added term. Among the rules that commute with multiplication by units and use the smallest added terms, the first with a single cycle (up to units) appears at $k = 4$, and it is unique there up to conjugation: $\alpha = 5+2i$ with added terms $\pm 1, \pm i, \pm(2+i), \pm(1-2i)$. Every other such rule with $k \le 5$ has at least two cycles, and we exhibit them; at $k = 6$, $22$ of the $112$ such rules have a single cycle up to norm $10^6$. The multiplier $3$ has norm $9$, which puts it at level $3$, where no such rule has a single cycle, so the single cycle of $3x+1$ does not survive at its natural Gaussian level. We checked that every orbit of the $5+2i$ rule starting at norm at most $3 \cdot 10^9$ reaches $1$ up to a unit, and conjecture that all orbits do. The rule $3z + \text{unit}$ converges on every start we tried but has five cycles; on the real line it applies $3x+1$ to whichever of $\pm x$ is $1 \bmod 4$, which forces division by $4$ at every step, so there it is provably convergent. We show that no rule in the family both agrees with $3x+1$ on the positive integers and contracts on average in the plane. Some rules appear to diverge, and in each such case the ratio $\alpha/(1+i)^k$ is real up to a unit. A random-walk model predicts the mean stopping time and the largest excursion seen. On $\mathbb{Z}[i]$, the map Joseph defined on the $2$-adic Gaussian integers in 1998 fails to reach a cycle within $20000$ steps for about $98\%$ of starts.

## 1. Introduction

The $3x+1$ map sends an even integer $x$ to $x/2$ and an odd one to $3x+1$. The Collatz conjecture says that every positive integer reaches $1$. The map has an accelerated form that acts only on odd numbers, $x \mapsto (3x+1)/2^v$ with $2^v$ the largest power of $2$ dividing $3x+1$. Each odd step multiplies $x$ by about $3$ and is followed by at least one division by $2$, so in the worst case $x$ grows by a factor $3/2$. On average the number of divisions is $2$, so on average $x$ shrinks by a factor $3/4$. Expansion in the worst case together with contraction on average is what makes the problem both plausible and open.

The Gaussian integers are the numbers $a+bi$ with $a, b \in \mathbb{Z}$, with norm $N(a+bi) = a^2+b^2$ and units $\pm 1, \pm i$. Two Gaussian integers are associates if one is a unit times the other. Factorization into primes is unique up to units, as in $\mathbb{Z}$. The prime $2$ is not a Gaussian prime: $2 = -i(1+i)^2$, and $1+i$, of norm $2$, is the prime that plays the part of $2$. The ring $\mathbb{Z}[i]/(1+i)$ has two elements, so every Gaussian integer is either divisible by $1+i$ or congruent to $1$ modulo it. We call $z$ **even** if $1+i$ divides $z$ and **odd** otherwise. Since

$$\frac{a+bi}{1+i} = \frac{(a+b) + (b-a)i}{2},$$

$a+bi$ is even exactly when $a+b$ is even. Dividing by $1+i$ halves the norm, as dividing by $2$ halves an integer. For an integer $n$, the exponent of $1+i$ in $n$ is twice the exponent of $2$ in $n$.

So the Gaussian integers support the same kind of map: divide by $1+i$ when possible, and otherwise multiply by a fixed odd $\alpha$ and add something that makes the result even. The question is which choices of $\alpha$ and of the added term behave like $3x+1$, and whether any of them has a single cycle. A rule on $\mathbb{Z}[i]$ is a pair of integer coordinates, so this is a two-dimensional Collatz problem, but the multiplication by $\alpha$ rotates as well as stretches. That rotation is what separates these maps from the lattice maps with integer multipliers studied by Kionke [2], where divergence on a positive proportion of starts is common.

Sections 2 to 4 set up the family, find the window in which a rule is Collatz-like, and present the main rule $H$. Section 5 classifies the rules that commute with units for $k \le 6$. Sections 6 and 7 treat the rule $3z + \text{unit}$ and the question of how ordinary $3x+1$ sits inside the Gaussian picture. Sections 8 to 10 cover divergence, a random-walk model and Joseph's map. The rest covers related work, methods and open problems.

## 2. The rule family

Fix an odd Gaussian integer $\alpha$, the multiplier, and an integer $k \ge 1$, the level. For an odd $z$ we want an added term $d(z)$ with

$$(1+i)^k \mid \alpha z + d(z).$$

This congruence says $d(z) \equiv -\alpha z \pmod{(1+i)^k}$, and the right-hand side depends only on the class of $z$ modulo $(1+i)^k$. So the class of $z$ decides which added terms work, and nothing else about $z$ matters. The ring $\mathbb{Z}[i]/(1+i)^k$ has $N\left((1+i)^k\right) = 2^k$ elements, half of them odd. Because $\alpha$ is odd it is invertible modulo $(1+i)^k$, so as $z$ runs over the $2^{k-1}$ odd classes, $-\alpha z$ does too. A rule therefore needs a digit set $D$: one odd Gaussian integer from each odd class, $2^{k-1}$ in all. We call the elements of $D$ digits, and $d(z)$ is the digit in the class of $-\alpha z$.

The rule is

$$T(z) = \begin{cases} z/(1+i) & \text{if } (1+i) \mid z, \\ \alpha z + d(z) & \text{otherwise.} \end{cases}$$

Write $v(w)$ for the exponent of $1+i$ in $w \ne 0$. The accelerated map on odd $z$ is

$$S(z) = \frac{\alpha z + d(z)}{(1+i)^{v(\alpha z + d(z))}},$$

which is odd again, and $v(\alpha z + d(z)) \ge k$ by construction. We count orbit lengths either in steps of $T$ or in odd steps, which are steps of $S$.

The level $k$ is a design parameter: each $k$ gives its own family of rules, with $2^{k-1}$ digits each. For $3x+1$ in $\mathbb{Z}$ the corresponding level is $k = 1$: there is one odd class modulo $2$, and the single digit $1$ makes $3x+1$ even. In $\mathbb{Z}[i]$ a larger $k$ is needed, for the reason given in Section 3.

A rule commutes with units if $T(\varepsilon z) = \varepsilon T(z)$ for every unit $\varepsilon$. This holds exactly when $D$ is closed under multiplication by units, since $\varepsilon d(z)$ is then the digit that works for $\varepsilon z$. Such a rule acts on associate classes, and its cycles come in sets of four associates; we count cycles up to units. If $D$ is also closed under complex conjugation, then $\alpha$ and $\bar\alpha$ give mirror-image rules.

*Proposition 1.* No rule with $k \le 2$ commutes with units.

*Proof.* Since $-1 - 1 = -2 = i(1+i)^2$, we have $-z \equiv z \pmod{(1+i)^2}$ for all $z$, so $z$ and $-z$ are in the same class modulo $(1+i)^k$ when $k \le 2$ and receive the same digit. Commuting with units requires $d(-z) = -d(z)$, so $d(z) = -d(z)$ and $d(z) = 0$, which is not odd. $\square$

For $k \ge 3$ the units are distinct modulo $(1+i)^k$ (their differences have $v \le 2$), so they act freely on the odd classes, which fall into $2^{k-3}$ orbits of four. A digit set that commutes with units is then the union of the unit multiples of one digit chosen for each orbit. We say a digit set is minimal if each chosen digit has the least norm in its class.

## 3. Which multipliers are Collatz-like

Each odd step multiplies the norm by about $N(\alpha)$ and is followed by at least $k$ divisions by $1+i$, each of which halves the norm.

*Proposition 2.* If $N(\alpha) < 2^k$, every orbit of $T$ is eventually periodic.

*Proof.* Let $\rho = |\alpha|/2^{k/2} < 1$ and $\delta = \max_{d \in D} |d|$. For odd $z$, $|S(z)| \le (|\alpha|\,|z| + \delta)/2^{k/2} = \rho|z| + \delta/2^{k/2}$. Let $R = \delta/\left(2^{k/2}(1-\rho)\right)$. If $|z| > R$ then $|S(z)| < |z|$, and if $|z| \le R$ then $|S(z)| \le R$. Even steps reduce $|z|$. Norms are nonnegative integers, so they cannot decrease forever, and every orbit enters the disc $|z| \le R$ and stays within a bounded set from then on. A bounded set of Gaussian integers is finite, so the orbit is eventually periodic. $\square$

Rules below $2^k$ are therefore decided by a finite computation; they are not Collatz-like. Above $2^k$ the worst case expands: an odd step that receives exactly $k$ divisions multiplies the norm by $N(\alpha)/2^k > 1$.

On average there are more divisions. Fix $j \ge 0$. As $z$ runs over the odd classes modulo $(1+i)^{k+j}$, the value $\alpha z + d(z)$ runs evenly over the classes of multiples of $(1+i)^k$ modulo $(1+i)^{k+j}$, and a fraction $2^{-j}$ of these are $0$. So among odd $z$ in the sense of density, $v(\alpha z + d(z)) - k$ has the geometric distribution $P(v - k \ge j) = 2^{-j}$, with mean $1$ and variance $2$. If the orbit visits classes as if at random, then per odd step

$$\mathbb{E}[\Delta \ln N] = \ln N(\alpha) - (k+1) \ln 2,$$

which is negative exactly when $N(\alpha) < 2^{k+1}$. This is the norm form of the growth-rate heuristic of Matthews and Watts [17], which generalizes conjectures of Möller [18]: for a map $x \mapsto (m_i x - r_i)/d$ on the classes $i$ modulo $d$ of $\mathbb{Z}$, almost all trajectories are expected to cycle if $|m_0 \cdots m_{d-1}| < d^d$ and to diverge if the product exceeds $d^d$ [5, Conjecture 3.1]. We call a rule **Collatz-like** if

$$2^k < N(\alpha) < 2^{k+1},$$

that is, expanding in the worst case and contracting on average. For $3x+1$ the same criterion reads $2 < 3 < 4$ with $k = 1$.

*Proposition 3.* There is no Collatz-like rule with $k = 1$.

*Proof.* The window is $2 < N(\alpha) < 4$, so $N(\alpha) = 3$. A sum of two squares is $0$, $1$ or $2$ modulo $4$, so $3$ is not a norm. $\square$

In other words, the direct analogue of $3x+1$, one class and one digit, does not exist in $\mathbb{Z}[i]$. The obstruction is the same arithmetic fact that keeps $3$ prime in $\mathbb{Z}[i]$. Alvarado and the students of a 2017 REU [16] studied the level-$1$ rule that sends an even $z$ to $z/(1+i)$ and an odd $z$ to $(2+i)z + u$, with $u$ a fixed unit, and found that about $5.6\%$ of the starts they tested with $|a|, |b| \le 100$ reached a unit within $10000$ steps [unverified: seen only in talk slides]. Since $N(2+i) = 5$ lies above the window $(2, 4)$, this rule expands on average by the criterion above, with drift $\ln 5 - 2\ln 2 > 0$ per odd step, which is consistent with that figure. Since $\alpha$ is odd, $N(\alpha) \equiv 1 \pmod 4$, which leaves these windows:

| $k$ | window | odd norms in window | $\alpha$ up to units |
|---|---|---|---|
| 1 | $(2, 4)$ | none | none |
| 2 | $(4, 8)$ | 5 | $2 \pm i$ |
| 3 | $(8, 16)$ | 9, 13 | $3$, $3 \pm 2i$ |
| 4 | $(16, 32)$ | 17, 25, 29 | $4 \pm i$, $5$, $4 \pm 3i$, $5 \pm 2i$ |
| 5 | $(32, 64)$ | 37, 41, 45, 49, 53, 61 | 11 values |
| 6 | $(64, 128)$ | 65 to 125 | 28 values |

At $k = 2$ the multiplier $2+i$ has the smallest odd prime norm, which makes it the closest analogue of $3$ as the smallest odd prime. Its rules have two digits and, by Proposition 1, cannot commute with units.

## 4. The rule H

The main rule has $\alpha = 5+2i$, $N(\alpha) = 29$ and $k = 4$, so $16 < 29 < 32$. Since $(1+i)^4 = -4$, classes modulo $(1+i)^4$ are classes modulo $4$. There are $8$ odd classes, in two orbits under the units: the classes of the units themselves, and the classes of the unit multiples of $1+2i$. The minimal digit set that commutes with units is

$$D = \{\pm 1,\ \pm i,\ \pm(2+i),\ \pm(1-2i)\},$$

the units together with the unit multiples of $2+i$. The rule $H$ is

$$H(z) = \begin{cases} z/(1+i) & \text{if } (1+i) \mid z, \\ (5+2i)z + d(z) & \text{otherwise,} \end{cases}$$

with $d(z) \in D$ the digit that makes $(5+2i)z + d(z)$ divisible by $4$. In full:

| $z \bmod 4$ | $1$ | $i$ | $-1$ | $-i$ | $1+2i$ | $2+i$ | $-1-2i$ | $2-i$ |
|---|---|---|---|---|---|---|---|---|
| $d(z)$ | $-1+2i$ | $-2-i$ | $1-2i$ | $2+i$ | $-1$ | $-i$ | $1$ | $i$ |

The table follows from one congruence. Modulo $4$, $\alpha = 5+2i \equiv 1+2i$, and $(1+2i)^2 = -3+4i \equiv 1$. If $z \equiv u$ for a unit $u$, the digit must be $\equiv -(1+2i)u$; no unit lies in that class, and its elements of least norm are the unit multiples of $2+i$, of norm $5$. If $z \equiv (1+2i)u$, the digit must be $\equiv -(1+2i)^2 u \equiv -u$, so it is the unit $-u$. Multiplication by $\alpha$ swaps the two orbits of classes, and the digits alternate accordingly between units and associates of $2+i$. For example, $(5+2i) \cdot 1 + (-1+2i) = 4+4i = -(1+i)^5$, so $S(1) = -1$, and by symmetry $S(-1) = 1$. The other minimal choice for the second orbit, the unit multiples of $1+2i$, gives the mirror image of the rule with $\alpha = 5-2i$.

Where $3x+1$ has one digit, $H$ has eight, because the window forces $k = 4$ and there are $2^{k-1} = 8$ odd classes modulo $(1+i)^4$. Every odd step of $H$ gains at least four factors of $1+i$, which divide the norm by $16$, against a factor of $29$; on average it gains five, which divide by $32$. So the norm changes on average by a factor $29/32$ per odd step, about $-0.14$ bits. For $3x+1$, counted on $x^2$ so that the two compare, the factor is $(3/4)^2 = 9/16$, about $-0.83$ bits. $H$ contracts about six times more slowly, so its orbits drift down weakly and can climb far before they fall, which is what the record excursions of Section 12 show.

A typical orbit rises and falls as $3x+1$ orbits do. Under $S$ the orbit of $3$ is

$$3 \to -4-i \to 4+3i \to -3+2i \to 2-3i \to -4+3i \to -2-i \to -i,$$

with norms $9, 17, 25, 13, 13, 25, 5, 1$ and $4, 4, 6, 5, 4, 7, 7$ factors of $1+i$ removed at the successive steps. Longer orbits are common: $6827+4464i$ takes $2042$ odd steps to reach a unit.

*Conjecture 1.* Every orbit of $H$ reaches $\pm 1$ or $\pm i$.

Up to units, the only cycle known is the fixed class of $1$; on $\mathbb{Z}[i]$ itself it is the pair of cycles through $\pm 1$ and through $\pm i$, exchanged by multiplication by $i$. Section 12 describes the check that every orbit with $N(z) \le 3 \cdot 10^9$ reaches this cycle. Over that range the longest stopping time, the number of $T$-steps until the norm first falls below the starting norm, is $13913$, at $6742+34011i$. The largest excursion, $\log_2$ of the peak norm before stopping divided by the starting norm, is $122.9$, at $17467+5226i$.

## 5. Rules that commute with units, $k \le 6$

By Proposition 1 the rules that commute with units start at $k = 3$. We take every Collatz-like $\alpha$ up to units and every minimal digit set that commutes with units. At $k = 3$ the units are a complete set of odd residues, so the digit set is the units. At $k = 4$ there are two minimal digit sets, units with the unit multiples of $2+i$ or of $1+2i$; they are complex conjugates, so up to conjugation each $\alpha$ gives one rule. At $k = 5$ the minimal digit set is unique: the unit multiples of $1$, $3$, $1+2i$ and $1-2i$. At $k = 6$ there are eight orbits of classes and four minimal digit sets.

The table lists each rule with $k \le 5$ up to conjugation, with cycles given by a representative in the first quadrant and the number of odd steps in the cycle up to units. Each cycle was found by iterating from small starts in exact integer arithmetic, choosing the digit directly from the congruence rather than from a table.

| $k$ | $\alpha$ | cycles found | two cycles (odd length) |
|---|---|---|---|
| 3 | $3$ | 5 | $1$ (1), $2+i$ (2) |
| 3 | $3+2i$ | 4 | $1$ (1), $3$ (4) |
| 4 | $4+i$ | 2 | $1$ (1), $2+i$ (1) |
| 4 | $4-i$ | 2 | $1$ (1), $2+i$ (1) |
| 4 | $5$ | 34 or more, and divergent orbits | $1$ (1), $2+i$ (1) |
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

The rules at $k = 5$ with $\alpha = 6-i$, $5-4i$, $6-3i$, $7-2i$ and $6-5i$ are the mirror images of those listed. The counts in the third column are for all starts with $N(z) \le 10^6$, and each is the same at $N(z) \le 10^5$. For $\alpha = 5$ the count covers only the starts that reached a cycle within $50000$ steps; the other $10.5\%$ did not. Two of the cycles are long: $5-2i$ at $k = 4$ has a cycle of $70$ odd steps through $15+16i$, and $6+5i$ at $k = 5$ has one of $130$ odd steps through $164+199i$ (for $6-5i$, through its mirror image $199+164i$).

What is proven, by exhibiting the cycles, is the following.

*Proposition 4.* Among Collatz-like rules with $k \le 5$ that commute with units and use a minimal digit set, every rule other than $H$ and its mirror image has at least two cycles up to units.

What is not proven is that $H$ has only one. That is Conjecture 1, supported by the check to norm $3 \cdot 10^9$. So $k = 4$ is the smallest level with a rule that commutes with units and appears to have a single cycle, and at that level the rule is unique up to conjugation. At $k = 3$ we also tried the non-minimal digit sets made of the unit multiples of $2+i$, $2-i$ or $3$; each has between $5$ and $14$ cycles up to units.

Uniqueness does not persist. At $k = 6$ there are $28$ multipliers up to units and four minimal digit sets, so $112$ rules, which conjugation pairs into $56$. No minimal digit set at $k = 6$ is closed under conjugation, so no rule is its own mirror image. Of the $112$ rules, $22$ have a single cycle up to units for all starts with $N(z) \le 10^6$, with $\alpha = 7 \pm 4i$, $8 \pm 3i$, $9 \pm 2i$, $10 \pm i$, $10 \pm 3i$ and $11 \pm 2i$. Four of them have $\alpha = 11 \pm 2i$, whose drift is only $\ln(125/128)$ per odd step, and their excursions pass $2^{200}$ in norm, which is the escape bound of `gcz`. For these four the stopping-time check of Section 12 was run from norm $1$, with escape at coordinates $2^{110}$. The $26$ starts that escaped even that bound were iterated in unbounded arithmetic until their norm fell below the starting norm, which took up to $138239$ steps of $T$ with excursions up to about $2^{300}$. The rules with $\alpha = 9$ and $\alpha = 11$ have orbits that appear to diverge. The remaining rules have at least two cycles. The same counts hold at $N(z) \le 10^5$.

### 5.1 Levels $0$ to $6$

The table collects, level by level, the rules that commute with units. A rule here has a Collatz-like multiplier, $2^k < N(\alpha) < 2^{k+1}$, and a minimal digit set in the sense of Section 2. For each orbit of the units on the odd classes modulo $(1+i)^k$, that set takes the unit multiples of one element of least norm in a class of the orbit, and every such choice is counted. The minimal sets are the units at $k = 3$; the units with the unit multiples of $2+i$, or of $1+2i$, at $k = 4$; the unit multiples of $1$, $3$, $1+2i$ and $1-2i$ at $k = 5$; and at $k = 6$ the unit multiples of $1$, $3$, $1 \pm 2i$ and $2 \pm 3i$, together with those of $1+4i$ or $1-4i$ and of $3+4i$ or $3-4i$. Rules are counted up to units, which replace $\alpha$ by an associate, and in parentheses also up to conjugation, which replaces $(\alpha, D)$ by $(\bar\alpha, \bar D)$. Cycles are counted up to units.

| $k$ | window | norms in window | rules | single cycle | cycles of the others | checked to |
|---|---|---|---|---|---|---|
| 0 | $(1, 2)$ | none | 0 | — | — | proof |
| 1 | $(2, 4)$ | none | 0 | — | — | proof |
| 2 | $(4, 8)$ | 5 | 0 | — | — | proof |
| 3 | $(8, 16)$ | 9, 13 | 3 (2) | 0 | 5, 4 | $10^9$ |
| 4 | $(16, 32)$ | 17, 25, 29 | 14 (7) | 1 ($H$) | 2, 2, 3, 3, 6; $\alpha = 5$: 34 or more, divergent | $10^6$; $H$ to $3 \cdot 10^9$ |
| 5 | $(32, 64)$ | 37, 41, 45, 49, 53, 61 | 11 (6) | 0 | 2, 3, 3, 4, 4, 18 | $10^6$ |
| 6 | $(64, 128)$ | 65, 73, 81, 85, 89, 97, 101, 109, 113, 117, 121, 125 | 112 (56) | 22 (11) | see below | $10^6$ |

The columns "single cycle" and "cycles of the others" count rules up to units and conjugation, except the $22$, which counts up to units only. "Checked to" is the bound on $N(z)$ up to which every start was followed to its cycle.

At $k = 0$ the condition $(1+i)^0 \mid \alpha z + d$ holds for every $d$, and there is one class, so a rule has a single digit. The window $1 < N(\alpha) < 2$ contains no integer, so no multiplier is Collatz-like. Level $0$ is trivial in a second sense as well. The only one-element set closed under units is $\{0\}$, so the rules that commute with units send an odd $z$ to $\alpha z$, which is odd again, and no odd orbit ever divides by $1+i$. For a unit $\alpha$ every odd orbit is then periodic, and otherwise every odd orbit grows without bound.

At $k = 1$ the window $(2, 4)$ contains only $3$, and $3$ is not a sum of two squares (Proposition 3). Separately, by Proposition 1 no rule at this level commutes with units.

At $k = 2$ the window contains the norm $5$, with $\alpha = 2 \pm i$, but by Proposition 1 no rule at this level commutes with units, with any digits. All the single-cycle rules at $k = 2$ found in the sweeps below are of the kind that does not commute with units. Nor can a rule at $k = 2$ commute with units in the weaker sense that $S(-z)$ is always an associate of $S(z)$. Since $z$ and $-z$ share a class, they share a digit $d$. The condition would make $(\alpha z + d)/(\alpha z - d)$ a unit times a power of $1+i$. For large $|z|$ this ratio is within $1/2$ of $1$ without equaling it, and no unit times a power of $1+i$ other than $1$ lies within $1/2$ of $1$.

At $k = 3$ the two rules are $G_3$, with five cycles, and $G_{3+2i}$, with four; $G_{3-2i}$ is the mirror image of $G_{3+2i}$. The non-minimal digit sets of Section 5 do not change the outcome either. With the unit multiples of $2+i$, $2-i$ or $3$ as digits, the three multipliers give between $5$ and $14$ cycles.

At $k = 6$ the $45$ rules up to conjugation with more than one cycle are distributed as follows. $19$ have $2$ cycles, $4$ have $3$, $3$ have $4$, $5$ have $5$, one each has $6$ and $7$, $4$ have $8$, $2$ have $9$ and $2$ have $10$. The other $4$ are the rules with $\alpha = 9$ and $\alpha = 11$, which have $23$ to $25$ cycles and orbits that appear to diverge. For $6$ of these $45$ rules, those with $\alpha = 10 \pm 5i$ and the two with $\alpha = 11 \pm 2i$ that have two cycles, some starts reached the step cap of $50000$, so their counts are lower bounds. This does not affect the count of $22$.

So level $4$ is the first level with a rule that commutes with units and has a single cycle, and $H$ is the only one there, up to conjugation. Levels $3$ and $5$ have none, and level $6$ has $22$ of $112$. The single-cycle property is common at level 6, so it does not pick out $H$ by itself. What singles out $H$ is the smallest level at which a single-cycle rule exists, together with uniqueness at that level.

Ordinary $3x+1$ has its own window at level $1$ in base $2$, since $2 < 3 < 4$. The Gaussian multiplier $3$ has norm $9$, and the windows are disjoint, so $9$ falls in exactly one of them: $8 < 9 < 16$, level $3$. At level $3$ no rule that commutes with units has a single cycle, with the minimal digits or with any of the non-minimal ones tried. So the single cycle conjectured for $3x+1$ on the positive integers does not survive at its natural Gaussian level. $G_3$ comes closest, with five cycles, and Section 6 shows that its real axis carries only an easy version of $3x+1$.

Symmetry under units also separates $H$ from $3x+1$. The map $x \mapsto 3x+1$ does not commute with $-1$. On the negative integers it acts as $3x-1$ does on the positive ones, and there it has three cycles: $-1 \to -2$ of $2$ steps, one through $-5$ of $5$ steps and one through $-17$ of $18$ steps. None is the negative of $1 \to 4 \to 2$, which has $3$ steps. So the single cycle conjectured for $3x+1$ holds only on the positive half of $\mathbb{Z} \setminus \{0\}$. A rule that commutes with units has no such half: every odd Gaussian integer is a unit times a primary one, and its orbit is the same unit times the orbit of that primary element, while an even nonzero element reaches an odd one by division. The point $0$ is fixed, as it is under $3x+1$, and is set aside in both. Conjecture 1 therefore asserts that on all of $\mathbb{Z}[i] \setminus \{0\}$ the only cycles of $H$ are the two through $\pm 1$ and through $\pm i$, each the image of the other under multiplication by $i$, and the check of Section 12 covers every nonzero starting point, not only the primary ones. In $\mathbb{Z}$ no rule with multiplier $3$ can commute with $-1$, for the reason of Proposition 1: the window $2 < 3 < 4$ forces $k = 1$, and $x \equiv -x \pmod 2$. Integer rules that commute with $-1$ exist from $k = 2$ on, for example with multiplier $5$ or $7$ and digits $\pm 1$, but not with multiplier $3$.

Rules that do not commute with units have a single cycle even more often. In sweeps with $N(z) \le 10^5$ and each digit chosen freely from odd elements of norm at most $5$, a single nonzero cycle appeared in $68$ of $288$ rules at $k = 2$ (with $\alpha$ an associate of $2 \pm i$), $108$ of $972$ at $k = 3$ and $76$ of $448$ at $k = 4$. Having one cycle is therefore not enough to pick out a rule. What distinguishes $H$ is symmetry under units together with the smallest level at which a single cycle occurs.

## 6. The rule $G_3$

The most direct analogue of $3x+1$ takes $\alpha = 3$ and $k = 3$:

$$G_3(z) = \begin{cases} z/(1+i) & \text{if } (1+i) \mid z, \\ 3z + u(z) & \text{otherwise,} \end{cases}$$

where $u(z)$ is the unit with $(1+i)^3 \mid 3z + u(z)$. Since $9$ lies in the window $(8, 16)$, this rule is Collatz-like, and its mean factor per odd step, $9/16 = (3/4)^2$, is the square of the factor for $3x+1$. All $3.14 \cdot 10^6$ starts with $N(z) \le 10^6$ reach one of five cycles up to units:

| cycle | odd length | share of starts |
|---|---|---|
| $2+i \to 1+2i$ | 2 | $92.3\%$ |
| $5+2i \to 2+5i$ | 2 | $5.6\%$ |
| $19+6i \to 10+19i \to 22+7i \to 11+22i \to 25+8i$ | 5 | $1.0\%$ |
| the mirror image of the last, through $6+19i$ | 5 | $1.0\%$ |
| $1$ | 1 | $0.2\%$ |

The stopping-time check of Section 12 extends this to all $N(z) \le 10^9$ with no new cycle, and the rule with $\alpha = 3+2i$ has exactly the four cycles of Section 5 over the same range. In this sense $G_3$ resembles $3x+1$ on all of $\mathbb{Z}$, which has the cycles through $1$, $-1$, $-5$ and $-17$ besides the fixed point $0$, more than $3x+1$ on the positive integers.

On the real line $G_3$ is ordinary $3x+1$ in disguise. Let $x$ be an odd integer. The digit must make $3x+u$ divisible by $(1+i)^3$. For an integer this means divisible by $4$, since the exponent of $1+i$ in an integer is even. A non-real unit gives $v(3x \pm i) = 1$, so $u = \pm 1$, and $3x + u = 3y+1$ up to sign, where $y$ is whichever of $x$ and $-x$ is $1 \bmod 4$. Dividing an integer $m$ by $(1+i)^{v(m)}$ gives $\pm m/2^{v_2(m)}$ or $\pm i\, m/2^{v_2(m)}$, so up to units $S(x) = |3y+1|/2^{v_2(3y+1)}$. The two axes, taken up to units, are invariant, and on them $G_3$ is the accelerated $3x+1$ map applied to the representative of $\pm x$ that is $1 \bmod 4$.

The choice of representative matters. Because $y \equiv 1 \pmod 4$, the number $3y+1$ is always divisible by $4$, so for $|x| > 1$

$$|S(x)| \le \frac{3|x|+1}{4} < |x|.$$

Every real orbit therefore decreases to $1$, and this is proven. The four extra cycles all lie off the axes. So $G_3$ extends the steps of $3x+1$ to the plane, but its real line carries an easy one-dimensional map, and the open part of the problem lies entirely off the line. This also explains why the share of starts that end at $1$ falls as the bound grows ($0.2\%$ at $10^6$, $0.07\%$ at $10^7$): the real line is a set of density zero, and off it most orbits reach $2+i$.

## 7. Is $3x+1$ a special case?

Is ordinary $3x+1$ a special case of some Gaussian map, obtained by collapsing the plane to a line? Section 6 shows one sense in which it is: the steps of $G_3$ on the real line are $3x+1$ steps. The arithmetic of $3$ in $\mathbb{Z}[i]$ is what allows it. The prime $3$ is $3 \bmod 4$, so it stays prime in $\mathbb{Z}[i]$ (it is inert), and multiplication by $3$ preserves the real line. By contrast $5 = (2+i)(2-i)$ splits, and its prime factors $2 \pm i$ are not real up to units, so a rule with $\alpha = 2+i$ rotates every odd step and has no invariant line. The multiplier of $H$, $5+2i$ with norm $29 = (5+2i)(5-2i)$, is likewise a split prime and leaves no line invariant.

But the line in $G_3$ does not carry the Collatz problem itself, and within the family it cannot.

*Proposition 5.* Let a rule have $\alpha = 3$ and level $k$.

1. If $k \ge 3$ and every odd integer receives an integer digit, then on the axes (up to units) the rule reduces $|x|$ at every odd step once $|x|$ exceeds the largest digit, so every real orbit is eventually periodic.
2. If the odd step at every positive odd integer $n$ is $n \mapsto 3n+1$, then $k \le 2$, and the rule is not Collatz-like: $N(3) = 9 > 2^{k+1}$, so the mean change of $\ln N$ per odd step in the model of Section 3 is at least $\ln 9 - 3\ln 2 > 0$.

*Proof.* For (1), an integer divisible by $(1+i)^3$ is divisible by $4$, so $|S(x)| \le (3|x| + |d|)/4$, which is less than $|x|$ when $|x| > |d|$. For (2), $(1+i)^k$ must divide $3 \cdot 3 + 1 = 10 = -i(1+i)^2(2+i)(2-i)$, so $k \le 2$. $\square$

So no rule in the family both agrees with $3x+1$ on the positive integers and contracts on average in the plane. Two further facts point the same way. Dividing by $(1+i)^2 = 2i$ turns the real axis into the imaginary one, so a rule that does not commute with units does not keep the real line, only the two axes, and digits chosen by residue class cannot see the sign of $x$: a class modulo $(1+i)^k$ that contains a positive odd integer also contains a negative one. Joseph's map (Section 10) avoids the rotation by dividing by $2$ instead of $1+i$, and it does restrict to $3x+1$ on $\mathbb{Z}$; as Proposition 5 predicts, it drifts outward in the plane.

*Open question.* Is there a Gaussian rule with a single cycle that restricts to $3x+1$ on the real line? Proposition 5 rules this out for one multiplier and one level. It leaves open the generalized maps of Matthews type, in which the multiplier, the level, or both depend on the residue class, together with normalizations that divide by $2$ where possible. A rule of that kind would need level at most $2$ on the class of $3 \bmod 4$ and could compensate with higher levels elsewhere. We have not tested any such rule. If one were found and appeared to have a single cycle, proving that would include proving the Collatz conjecture, so the most one could hope for is computational evidence and a reduction.

## 8. Divergence

Among the Collatz-like rules with $k \le 6$ that we ran, orbits that appear to diverge occurred only for $\alpha = 5$ at $k = 4$ and $\alpha = 9, 11$ at $k = 6$. These are the cases in which

$$\lambda = \frac{\alpha}{(1+i)^k}$$

is real up to a unit. Since $(1+i)^2 = 2i$, this happens exactly when $\alpha$ is real up to a unit and $k$ is even; in the windows that gives $5$ at $k = 4$ (where $\lambda = -5/4$) and $9, 11$ at $k = 6$. The Collatz-like condition makes $|\lambda| = \sqrt{N(\alpha)/2^k} > 1$. An odd step that receives exactly $k$ divisions multiplies $z$ by $\lambda$ plus a bounded error. When $\lambda$ is real up to a unit, a run of such steps keeps $z$ pointing along a fixed set of directions while $|z|$ grows by $|\lambda|$ each time. When $\lambda$ is not real up to a unit, the direction keeps turning, which breaks up any such run.

For $\alpha = 5$ the mechanism is visible. Of about $1500$ random starts with norm up to $1.8 \cdot 10^7$, $176$ led to apparently divergent orbits, and all of them settled onto the lines $x - y = \pm 1$ or $x + y = \pm 1$, with ratio $-5/4$ per odd step. These lines are nearly invariant: $876$ of $1000$ sample points on them map back onto them. On them the rule acts like a one-dimensional $5x+1$ map with exactly four divisions per odd step, and $5/4 > 1$. This is the analogue on lines of Kionke's divergent cones [2]. Matthews [5, Example 10.1] reports a similar picture for the extension of $3x+1$ to $\mathbb{Z}[\sqrt 2]$, whose multiplier $3$ is real: finitely many cycles found, three one-dimensional invariant sets, and apparently divergent orbits. Every digit set we tried with $\alpha = 5$, $9$ or $11$ had such orbits. The divergence is not proven (Section 13).

The ratio itself is what matters, not its square. For $\alpha = 3$ at $k = 3$ and $\alpha = 7$ at $k = 5$, $\lambda^2$ is real up to a unit but $\lambda$ is not, and both rules converged on every start tried.

## 9. A random-walk model

Treat the excess divisions $v - k$ at successive odd steps as independent geometric variables with $P(v - k \ge j) = 2^{-j}$. Then $\log_2 N$ performs a random walk whose odd steps add $\log_2 N(\alpha)$ and whose divisions each subtract $1$. For $H$ the drift per odd step is

$$\log_2 29 - 5 = -0.142 \text{ bits},$$

with variance $2$.

Two predictions follow. The first is the mean stopping time. Simulating $4 \cdot 10^6$ walks of this kind, counting $T$-steps until the walk first falls below its start, gives a mean of $53.8$. The check of Section 12 measured between $53.82$ and $53.93$ in each of its $30$ chunks.

The second is the largest excursion. For a walk with negative drift, the probability that the maximum before stopping exceeds $h$ bits is about $C e^{-\theta h}$, where $\theta$ solves $\mathbb{E}[e^{\theta X}] = 1$ for the per-odd-step change $X$. Here $X = \log_2 29 - 4 - G$ with $G$ geometric, which gives $\theta = 0.165$ per bit; the diffusion approximation $2\mu/\sigma^2 = 0.142$ is a little low. The simulation gives $C \approx 0.36$, counting the peak just after each multiplication and before the divisions. Over $n$ independent starts the record excursion then has median about

$$\frac{\ln n + \ln C - \ln \ln 2}{\theta},$$

which for the $n = 1.18 \cdot 10^9$ starts of the check is $122.4$ bits. The observed record is $122.9$ bits. The agreement is closer than the model can justify, because the starts are not independent. The record in $14$ of the $30$ chunks comes from orbits that pass through the same point, of norm about $2^{151.2}$: we checked this directly for five of them, and the others fit by adding the starting norm to the excursion. So the largest excursions over the whole range come from one deep orbit that many starts join, and the record tests the model only roughly. The mean stopping time is the better test.

(An earlier estimate in the working notes used $\theta = 0.142$ and $n = 4 \cdot 10^7$, the size of one chunk, and also arrived at about $123$ bits. The figures above replace it.)

## 10. Joseph's map

Joseph [1] defined a map on $\mathbb{Z}_2[i]$, the Gaussian integers completed at $2$:

$$T_J(z) = \begin{cases} z/2 & \text{if } z \equiv 0 \pmod 2, \\ (3z + \beta)/2 & \text{if } z \equiv \beta \pmod 2,\ \beta \in \{1, i, 1+i\}. \end{cases}$$

On $\mathbb{Z}_2$, the real elements, $T_J$ is the $3x+1$ map $x \mapsto x/2$ or $(3x+1)/2$. Joseph showed that $T_J$ is chaotic in the sense of Devaney and topologically, though not algebraically, conjugate to the product of the $2$-adic $3x+1$ map with itself. His subject was the dynamics on the $2$-adic completion, not convergence of orbits of Gaussian integers, and he computed no orbits in $\mathbb{Z}[i]$. Kucinski [15] studied $T_J$ on $\mathbb{Z}[i]$. She bounded the period of a cycle in terms of its parity vector, showed that the second and fourth quadrants contain no cyclic points, and determined all cycles of period at most $400$, of which there are $77$ [unverified: read through a text extraction]. We have not compared the cycles found below with her list.

$T_J$ maps $\mathbb{Z}[i]$ into itself, so one can ask what it does there. If the classes modulo $2$ are visited evenly, the mean change of $\ln N$ per step is $\frac{3}{4}\ln N(\alpha) - \ln 4$, which is $+0.26$ for $\alpha = 3$. Of the $314197$ starts with $N(z) \le 10^5$, $308624$ ($98.2\%$) either had not reached a cycle after $20000$ steps or had a coordinate exceed $2^{100}$; the rest fell into $67$ cycles. The data are consistent with divergence of almost all orbits, though we have not proven it for any orbit. Of the same form, only multipliers of norm $5$ contract on average: with $\alpha = 1+2i$ or $1-2i$ (drift $-0.18$) all $16$ choices of small $\beta$ converged on every start with $N(z) \le 10^5$, and $8$ of them have a single cycle, for example $\beta = 1, i, -1+i$ for the classes $1, i, 1+i$, whose cycle passes through $1$; with $\alpha = 2 \pm i$ each rule has $3$ or $4$ cycles.

## 11. Related work

We found no published study of an integral map on $\mathbb{Z}[i]$ that divides by $1+i$ at a level $k \ge 2$ and asks whether all orbits converge [unverified: based on a search of arXiv, journals and the bibliographies below, which may have missed something; parts of [3] and several web pages could not be read]. Two earlier studies come close. Kucinski [15] studied Joseph's map, which divides by $2$, on $\mathbb{Z}[i]$ (Section 10). Alvarado and the students of a 2017 REU [16] studied the level-$1$ rule with $\alpha = 2+i$ (Section 3); as far as we found, that work is unpublished. Chamberland's survey [19] records Joseph and Kucinski.

The family of Section 2 is a special case of several general frameworks, none of which works out Gaussian examples. Matthews [5, Section 10] considers generalized maps on the ring of integers of a number field modulo a non-unit $d$, and $T$ is one with $d = (1+i)^k$. Kohl's RCWA package [20] handles residue-class-wise affine maps of $\mathbb{Z}^2$ whose pieces have $2 \times 2$ integer matrix multipliers and lattice moduli, and each step of $T$ is such a map [unverified: taken from the package source; no worked $\mathbb{Z}^2$ example seen]. Siegel's Hydra maps [6] cover maps on rings of integers in general but give no examples in $\mathbb{Z}[i]$. Kionke [2] proposes replacing his scalar multipliers by integer matrices and leaves that open; the Gaussian rules are of that kind.

Joseph [1] is discussed in Section 10. Lagarias's annotated bibliographies [3, 4] list the generalizations of $3x+1$ to other rings. Leigh is cited by Siegel [6] as the first to consider Collatz-type maps on rings of algebraic integers, and Matthews [5] followed with maps on $\mathbb{Z}[\sqrt 2]$ and on polynomial rings over finite fields, with conjectures on the behaviour of divergent orbits; as reported by Siegel, Matthews knew of no other work on rings of integers. We have not seen Leigh's work [unverified]. Kionke [2] studies maps on lattices $\mathbb{Z}^e$ and on $\mathbb{Z}[\sqrt 2]$ of the form $x \mapsto (m_w x + r_w)/d$ on each coset, with positive integer multipliers. He proves that tame cones give divergent orbits of positive lower density, that for $\mathbb{Z}[\sqrt 2]$ at least half of all points diverge, and that for $d \ge 3$ a map can have finite stopping time on a set of density one and still diverge on a set of positive density. His multipliers are scalars and do not rotate, which is the difference from the rules here; the divergent rules of Section 8 are exactly those whose effective multiplier $\lambda$ is a scalar up to a unit. Kionke cites Kucinski [15] for a similar observation on $\mathbb{Z}[i]$, namely her result that the second and fourth quadrants contain no cyclic points of Joseph's map.

Garg [7] describes affine rules on four classes of Gaussian integers, with dyadic denominators, a periodic orbit of length $40$ and numerical Lyapunov exponents; the maps are not integral on $\mathbb{Z}[i]$. We have seen only a summary [unverified]. Aliyev [8] studies rational cycles of maps $(p_i x + k_i)/q$ and lists a Gaussian version as future work. Polynomial analogues over $\mathbb{F}_q[x]$, where all orbits are eventually periodic, form a separate line of work.

## 12. Computation and verification

The program `gcz.c` classifies every $z$ with $N(z) \le B$. It stores one entry per lattice point, follows each orbit until it reaches a point already classified, closes a cycle (found with Brent's method) or reaches a step cap, and fills in the result along the path. Coordinates are 128-bit integers, with an escape bound of $2^{100}$. The rule is given as a table over the residues of $(a \bmod 4, b \bmod 4)$, or modulo $8$ for $k \ge 5$.

For rules that commute with units, `gstop.c` checks a larger range by induction. For one associate of each odd $z$ with $10^7 < N(z) \le B$ (those with $a > 0$, $b \ge 0$), it iterates $T$ until the norm falls below $N(z)$, and it reports any orbit that returns to an associate of its start instead. Even $z$ need no check, since $N(z/(1+i)) < N(z)$. If every start in the range falls below its own norm, then by induction every orbit with $N(z) \le B$ ends in a cycle whose least element has norm at most $10^7$, and those cycles are known completely from `gcz`. For $H$ the run covered $B = 3 \cdot 10^9$ in $30$ chunks, $1.18 \cdot 10^9$ starts, with no failures and no new cycle; it took about $15$ minutes on one core. Comparisons of norms use exact 128-bit arithmetic while coordinates stay below about $2^{60}$, and the escape bound is $2^{110}$. The largest coordinates met were below $2^{80}$.

The Python module `exact.py` iterates the same rules in unbounded integer arithmetic, choosing each digit directly from the congruence instead of from the table. It re-derives every cycle reported here and agrees with `gcz` and `gstop` on the extreme orbits. For this draft we re-ran the cycle search of Section 5 for $k \le 5$, the orbits quoted in Sections 4 and 9, and the Joseph runs of Section 10, and computed $\theta$ and $C$ of Section 9. We also re-ran the sweeps behind Section 5.1 for $k = 3$ to $6$ at $N(z) \le 10^5$ and $10^6$ (`sweep_eq2.py`). We extended $G_3$ and $G_{3+2i}$ to $10^9$ with `gstop`, finding $3.92 \cdot 10^8$ starts each, no failures and no new cycle, with logs in `research/collatz/logs/`.

## 13. Open problems

1. Prove that $H$ has a single cycle up to units, or find another. A cycle with $n$ odd steps and total division exponent $V$ must satisfy two conditions: $2^V$ is close to $29^n$, and the argument of $(5+2i)^n$ is close to $V\pi/4$ modulo $\pi/2$. The second condition has no analogue for $3x+1$. Bounds of the Steiner and Simons–de Weger type [11, 12] adapted to both conditions might exclude cycles with few odd steps.
2. Prove that the rule with $\alpha = 5$ at $k = 4$ has divergent orbits. The lines $x \pm y = \pm 1$ are nearly invariant, and an exact description of the points that stay on them would give the proof.
3. Prove a density theorem in $(1+i)$-adic form. Terras [9] and Everett [10] showed that almost every positive integer has a finite stopping time under $3x+1$, because the parity pattern of the first $n$ steps depends only on $x \bmod 2^n$. The analogous pattern for these rules depends only on $z$ modulo a power of $1+i$, so we expect the same argument to give finite stopping time on a set of density one whenever $N(\alpha) < 2^{k+1}$; this has not been worked out. Such a theorem would not rule out divergence, since Kionke's examples show that density-one stopping and positive-density divergence can coexist in dimension two. Nichols [21] proved a Terras-type theorem for analogues of $3x+1$ in function rings of curves over $\mathbb{F}_2$ [unverified: only the abstract seen]. Siegel and O'Dwyer [22] prove a theorem of Tao type, almost bounded orbits on a set of logarithmic density one, for Hydra maps on rings of integers under a dimension constraint; we have not checked whether $H$, or an iterate of it, satisfies their hypotheses.
4. Study $H$ on the $2$-adic Gaussian integers. Since the digit depends only on $z \bmod 4$, $H$ extends continuously to $\mathbb{Z}_2[i]$. For $3x+1$ the $2$-adic extension preserves Haar measure and is conjugate to the shift [13, 14]. Whether the analogues hold for $H$ is open.
5. Push the check of $H$ to $10^{11}$ and beyond. At about $1.2 \cdot 10^6$ starts per second per core, $10^{11}$ needs about nine core-hours.
6. Decide the open question of Section 7: whether a rule with class-dependent multipliers or levels can restrict to $3x+1$ on the real line and have a single cycle.

## References

1. J. A. Joseph, A chaotic extension of the $3x+1$ function to $\mathbb{Z}_2[i]$, *Fibonacci Quarterly* 36(4) (1998), 309–316.
2. S. Kionke, A geometric approach to divergent points of higher dimensional Collatz mappings, arXiv:1511.05893 (2015); *Monatshefte für Mathematik* 182 (2017) [unverified: journal details from a secondary source].
3. J. C. Lagarias, The $3x+1$ problem: an annotated bibliography (1963–1999), arXiv:math/0309224.
4. J. C. Lagarias, The $3x+1$ problem: an annotated bibliography, II (2000–2009), arXiv:math/0608208.
5. K. R. Matthews, Generalized $3x+1$ mappings: Markov chains and ergodic theory, in J. C. Lagarias (ed.), *The Ultimate Challenge: The $3x+1$ Problem*, American Mathematical Society, 2010, 79–103. Preprint: http://www.numbertheory.org/PDFS/matthews-final-revised.pdf.
6. M. C. Siegel, $(p,q)$-adic analysis and the Collatz conjecture, arXiv:2412.02902.
7. Garg, Zenodo, doi:10.5281/zenodo.19078383 (March 2026) [unverified: seen only through a summary].
8. Aliyev, *Notes on Number Theory and Discrete Mathematics* 31(3) (2025), 658–666.
9. R. Terras, A stopping time problem on the positive integers, *Acta Arithmetica* 30 (1976), 241–252.
10. C. J. Everett, Iteration of the number-theoretic function $f(2n) = n$, $f(2n+1) = 3n+2$, *Advances in Mathematics* 25 (1977), 42–45.
11. R. P. Steiner, A theorem on the Syracuse problem, *Proceedings of the 7th Manitoba Conference on Numerical Mathematics* (1977), 553–559.
12. J. Simons and B. de Weger, Theoretical and computational bounds for $m$-cycles of the $3n+1$ problem, *Acta Arithmetica* 117 (2005), 51–70.
13. J. C. Lagarias, The $3x+1$ problem and its generalizations, *American Mathematical Monthly* 92 (1985), 3–23.
14. D. J. Bernstein, A non-iterative $2$-adic statement of the $3N+1$ conjecture, *Proceedings of the American Mathematical Society* 121 (1994), 405–408.

15. G. I. Kucinski, Cycles for the $3x+1$ map on the Gaussian integers, preprint, University of Scranton, May 2000. https://monks.scranton.edu/files/pubs/kucinski.pdf
16. A. Alvarado, The Collatz conjecture over the Gaussian integers, talk slides, University of Colima, 2 April 2025, on work of a 2017 REU at CSU Channel Islands. https://fefoaranda.com/wp-content/uploads/2025/04/colima-alvarado-2025.pdf [unverified: slides only; the paper, said to be submitted to the *Minnesota Journal of Undergraduate Mathematics*, was not found].
17. K. R. Matthews and A. M. Watts, A generalization of Hasse's generalization of the Syracuse algorithm, *Acta Arithmetica* 43 (1984), 167–175.
18. H. Möller, Über Hasses Verallgemeinerung des Syracuse-Algorithmus (Kakutanis Problem), *Acta Arithmetica* 34 (1978), 219–226.
19. M. Chamberland, An update on the $3x+1$ problem. https://chamberland.math.grinnell.edu/papers/3x_survey_eng.pdf
20. S. Kohl, RCWA: Residue-Class-Wise Affine Groups, GAP package; see also S. Kohl, *Restklassenweise affine Gruppen*, Dissertation, Universität Stuttgart, 2005.
21. D. Nichols, Analogues of the $3x+1$ problem in polynomial rings of characteristic $2$, *Experimental Mathematics* (2016), arXiv:1610.02545.
22. M. Siegel and R. O'Dwyer, Almost-boundedness of Collatz-type maps on number rings via Archimedean methods, arXiv:2609.33018 (2026).

[unverified: references 1, 2 (arXiv version), 5, 15 and 19 were read through text extraction. The titles, page ranges and initials in references 8, 9–14, 17 and 18 are given from memory, from the working notes or from other papers' bibliographies, and have not been checked against the publications.]
