# Invariants of Collatz maps across rings of integers

*Draft, 5 November 2026. Code, logs and Lean proofs: `research/collatz/`. Each claim carries its status: proved (on paper), Lean (proved in Lean 4), computed to a stated bound, computer-assisted, heuristic, literature, or open. Items marked [unverified] have not been checked against their source. The companion paper [Paper 2] cites this one for its definitions and for the results of Sections 2 and 6.*

## Abstract

The $3x+1$ map lives on the positive integers. We place it in a family of maps over a ring of integers $R$: a map multiplies an odd element by a fixed $\alpha$, adds a digit chosen by the class of the element modulo $\pi^k$ for a prime $\pi$ above $2$, and divides by $\pi$ while it can. A Collatz map is one in which every nonzero orbit reaches a cycle through a unit. We climb from $\mathbb{N}$ to $\mathbb{Z}$ and to the Gaussian integers $\mathbb{Z}[i]$ as a search for invariants: at each step some quantities of the ring change, and we ask which of them keep controlling the behaviour of the maps. Other rings serve as tests of the answers. The window $q^k < N(\alpha) < q^{k+1/(q-1)}$, with $q$ the size of the residue field, persists in every ring; it is the Matthews and Watts growth criterion written with norms. On $\mathbb{Z}$ the second unit $-1$ makes $3x \pm 1$ fail, and from modulus $4$ on the smallest digits give maps that commute with $-1$, among them Barina's $7x \pm 1$. On $\mathbb{Z}[i]$ the modulus of $3x+1$ is impossible because $3$ is not a sum of two squares, Collatz maps appear at the next modulus, and none of them commutes with the units. The Eisenstein integers $\mathbb{Z}[\omega]$, the one test ring with data, confirm that the absence of commuting maps at the first Gaussian modulus comes from ramification and not from the second dimension: there a Collatz map that commutes with the units appears at the first modulus that has multipliers. Four further rings that would separate invariants changed together are proposed, untested. The general results that explain these findings are stated once for any such ring: which moduli carry multipliers, how many trivial cycles a map can have, the modulus from which maps can commute with a finite unit group (none if the unit group is infinite), the unit cycles of such maps, rotated and mirrored maps, and forced fixed points.

## 1. Introduction

### 1.1 The lineage

The $3x+1$ map sends an even positive integer $x$ to $x/2$ and an odd one to $3x+1$, and the Collatz conjecture says that every positive integer reaches $1$ [Lagarias 1985; Lagarias 2010]. The same rule makes sense on $\mathbb{Z}$, where it has more cycles. Besides $0$ and $1 \to 4 \to 2$ it has $-1 \to -2$, a cycle of $5$ steps through $-5$ and one of $18$ steps through $-17$, and the conjecture of Böhm and Sontacchi says that these are all [Lagarias 1985]. Since $-(3x+1) = 3(-x) - 1$, the map acts on the negative integers as $3x - 1$ acts on the positive ones [OEIS A037084].

The growth criterion that this paper uses in every ring comes from the generalized maps on $\mathbb{Z}$. Hasse's maps multiply by $m$, add the residue that makes the result divisible by $d$, and divide by $d$ [Hasse 1975, as annotated in Lagarias 2003, entry 76], and Heppner showed that almost all iterates decrease when $m < d^{d/(d-1)}$ and increase when $m > d^{d/(d-1)}$ [Heppner 1978, as annotated in Lagarias 2003, entry 78]. Matthews and Watts studied maps $x \mapsto (m_i x - r_i)/d$ on the classes $i$ modulo $d$, first with every $m_i$ prime to $d$ [Matthews and Watts 1984] and then under a weaker condition [Matthews and Watts 1985]. Their Conjectures 1 and 2 [Matthews and Watts 1985, p. 30] say that if a weighted product of the ratios $m_i/d$ is below $1$, all trajectories are eventually periodic, and if it is above $1$, almost all diverge; their multipliers are positive. Leigh allowed any nonzero multipliers, built the Markov chain on residue classes that underlies the weights, and proved a growth formula for divergent trajectories [Leigh 1986, Lemma 3]. Section 2 uses these results.

The second unit of $\mathbb{Z}$ enters through Barina's $7x \pm 1$ map [Barina 2018]: $n \mapsto n/2$ for even $n$, $7n + 1$ for $n \equiv 1 \pmod 4$ and $7n - 1$ for $n \equiv 3 \pmod 4$. Barina conjectured that every positive orbit reaches $1$, checked this to $10^{15}$, and noted that the orbits of negative numbers are the negatives of those of positive numbers [Barina 2018; OEIS A317640]. Other maps on $\mathbb{Z}$ with this property are known: Collatz's original permutation [Tremblay 2019], Conway's amusical permutation [OEIS A006368; Matthews and Watts 1985, Example 7], and Matthews's maps $L_d$ with least absolute residues [Carnielli 2015; Tremblay 2019].

Other rings appear in three lines of work. Matthews's chapter treats maps on the ring of integers of a number field modulo a non-unit, with examples on $\mathbb{Z}[\sqrt 2]$ and $\mathbb{Z}[\sqrt 3]$ and no general conjecture, and remarks that rings other than $\mathbb{Z}$ invite systematic investigation [Matthews 2010, Sections 10 and 11]. Siegel reports that Leigh was the first to consider such maps, with the map $z \mapsto z/\sqrt 2$ on the class $0$ and $z \mapsto (3z+1)/\sqrt 2$ on the class $1$ modulo $\sqrt 2$ in $\mathbb{Z}[\sqrt 2]$, which is Matthews's Example 10.1; Siegel dates it 1983 in one paper and 1981 in another [Siegel 2021; Siegel 2024, Section 1.1.2]. Leigh's published paper is on $\mathbb{Z}$ only [Leigh 1986]. On the Gaussian integers, Joseph extended $3x+1$ to $\mathbb{Z}_2[i]$ with division by $2$ [Joseph 1998], Kucinski found cycles of that map on $\mathbb{Z}[i]$ and symmetries between parts of it [Kucinski 2000], and Alvarado and the students of a 2017 REU studied the map that divides by $1+i$ and otherwise multiplies by $2+i$ and adds a unit [Alvarado 2025; unverified: talk slides only]. Kionke [Kionke 2017] and Siegel [Siegel 2024] give frameworks on lattices and on rings of integers.

This paper takes the ring as a parameter. It defines one family of maps over $\mathbb{Z}$, $\mathbb{Z}[i]$ and $\mathbb{Z}[\omega]$, with the modulus and the digit table as parameters, and follows the family up the rings. As far as our search reached, no earlier work does this [unverified: parts of Lagarias's bibliographies and several of Matthews's web pages could not be read].

### 1.2 The questions

The climb is a search for invariants. Each ring brings its own values of a few quantities: the size of the residue field, the number of units, how close a unit other than $1$ can be to $1$, the shape of the lattice, how $2$ factors, and the automorphisms of the ring. When a behaviour of the maps changes from one ring to the next, the quantities that changed are the candidates for its cause. At each step we ask what persists from the smaller ring, what stops mattering, and what becomes important, including properties that make no sense to ask in $\mathbb{N}$: whether a map commutes with the units, which units its trivial cycles pass through, the angle of the multiplier, and how a map relates to its mirror image.

Section 2 sets up the family and the window. Sections 3 and 4 take the steps from $\mathbb{N}$ to $\mathbb{Z}$ and from $\mathbb{Z}$ to $\mathbb{Z}[i]$, the core of the climb, and report what each step finds; the Gaussian maps are studied in depth in [Paper 2]. Section 5 tests what the climb found in other rings: the Eisenstein integers, for which we have data, and four rings that would separate invariants that $\mathbb{Z}[i]$ changes together, each with the prediction it would test. Section 6 states the general results that explain what the climb found, for any ring of the kind considered, names the invariant behind each and the ring that tests it. Section 7 lists open questions.

### 1.3 Glossary

These are the paper's terms in brief, with the invariants explained in plain terms. Section 2 gives the formal definitions.

- A **unit** is an element with a multiplicative inverse in the ring. The units of $\mathbb{Z}$ are $\pm 1$, those of $\mathbb{Z}[i]$ are $\pm 1, \pm i$, and those of $\mathbb{Z}[\omega]$, with $\omega = e^{2\pi i/3}$, are the six elements $\pm 1, \pm\omega, \pm\omega^2$. In $\mathbb{N}$ the only unit is $1$. Two elements are **associates** if one is a unit times the other.
- The **norm** $N(z)$ measures size: $N(x) = |x|$ on $\mathbb{Z}$, $N(a+bi) = a^2 + b^2$ on $\mathbb{Z}[i]$, and $N(a + b\omega) = a^2 - ab + b^2$ on $\mathbb{Z}[\omega]$. It is multiplicative, it is $1$ exactly on the units, and $N(z)$ is the number of classes modulo $z$.
- The **prime above $2$** is a prime $\pi$ that divides $2$: $\pi = 2$ in $\mathbb{Z}$ and $\mathbb{Z}[\omega]$, and $\pi = 1+i$ in $\mathbb{Z}[i]$. An element is **odd** if $\pi$ does not divide it and **even** otherwise.
- The **residue field size** $q = N(\pi)$ is the number of classes modulo $\pi$. It is $2$ in $\mathbb{Z}$ and $\mathbb{Z}[i]$, where every element is even or odd, and $4$ in $\mathbb{Z}[\omega]$, where there are three odd classes modulo $2$. Dividing by $\pi$ divides the norm by $q$.
- **How $2$ factors.** In $\mathbb{Z}[i]$, $2 = -i(1+i)^2$ is a unit times the square of a prime: $2$ is **ramified**. In $\mathbb{Z}[\omega]$, $2$ stays prime: it is **inert**. In some rings $2$ is a product of two different primes: it **splits**. The exponent of $\pi$ in $2$ is the **ramification index** $e$, which is $2$ in $\mathbb{Z}[i]$ and $1$ in $\mathbb{Z}$ and $\mathbb{Z}[\omega]$.
- The **valuation** $v(z)$ is the exponent of $\pi$ in $z$. For a unit $u \ne 1$, $v(u-1)$ says how far the classes modulo powers of $\pi$ must be refined before $u$ and $1$ fall into different classes. We write $\kappa$ for the largest of these values: $\kappa = 1$ in $\mathbb{Z}$ ($v(-2) = 1$), $\kappa = 2$ in $\mathbb{Z}[i]$ ($v(-2) = 2$), and $\kappa = 1$ in $\mathbb{Z}[\omega]$. As long as $k \le \kappa$, some unit $u \ne 1$ has $uz$ in the same class as $z$ modulo $\pi^k$ for every odd $z$.
- The **lattice shape.** The elements of the ring are points: a line for $\mathbb{Z}$, a square grid for $\mathbb{Z}[i]$, a triangular grid for $\mathbb{Z}[\omega]$. The multiples of $\pi^k$ form a coarser grid of the same kind, and a class modulo $\pi^k$ is a translate of it. The smallest element of a class is the point of the class nearest to $0$; two points of a class can tie only if they lie on the boundary of the **Voronoi cell** of the coarser grid, the region of points nearer to $0$ than to any other grid point.
- An **automorphism** is a bijection of the ring that preserves sums and products. On $\mathbb{Z}[i]$ and $\mathbb{Z}[\omega]$, complex conjugation $z \mapsto \bar z$ is one. It may move the prime: $\overline{1+i} = 1 - i = -i(1+i)$ is $\pi$ times a unit other than $1$, while $\bar 2 = 2$.
- A **map** is a multiplier $\alpha$, a modulus $\pi^k$ and a **digit table**, which gives for each odd class modulo $\pi^k$ the number added after multiplying. The map $T$ divides an element by $\pi$ when it can and otherwise multiplies by $\alpha$ and adds the digit. The accelerated map $S$ does one multiplication and all the divisions that follow. The **smallest digits** are the elements of least norm in their classes.
- A **trivial cycle** is a cycle that contains a unit. A **unit cycle** is a cycle made only of units. A **Collatz map** is a map in which every nonzero orbit reaches a trivial cycle; it has no other cycle and no divergent orbit.
- A map **commutes with the units** if $T(uz) = uT(z)$ for every unit $u$. In $\mathbb{Z}$ this says that $T$ is an odd function.
- The **rotated** maps of a map replace $\alpha$ by $u\alpha$ for a unit $u$, and the **mirrored** map replaces $\alpha$ by $\bar\alpha$, each with the digit table carried along (Section 6.5).
- The **window** is the range of multipliers for which a map expands in the worst case and contracts on average (Section 2.2).
- The **drift** is the mean change of $\log_2 N$ per odd step, if the orbit visits the residue classes as a random element would. The **score** is $1/M - 1/N$, where $N$ is the norm of the multiplier and $M$ that of the modulus.

The invariants of the four rings, with the values that Sections 3 to 6 use:

| ring | units | $\pi$ | $q$ | $2$ | $e$ | $\kappa$ | lattice | conjugation fixes $\pi$ |
|---|---|---|---|---|---|---|---|---|
| $\mathbb{N}$ | $1$ | $2$ | $2$ | prime | 1 | none | half-line | (no conjugation) |
| $\mathbb{Z}$ | $\pm 1$ | $2$ | $2$ | prime | 1 | 1 | line | (no conjugation) |
| $\mathbb{Z}[i]$ | 4 | $1+i$ | $2$ | ramified | 2 | 2 | square | no: $\bar\pi = -i\pi$ |
| $\mathbb{Z}[\omega]$ | 6 | $2$ | $4$ | inert | 1 | 1 | triangular | yes |

## 2. Maps over a ring of integers

### 2.1 The family

Let $R$ be $\mathbb{Z}$ or the ring of integers of an imaginary quadratic field, with norm $N$: $N(x) = |x|$ on $\mathbb{Z}$ and $N(z) = |z|^2$ otherwise. The norm is multiplicative, $N(z) = 1$ exactly when $z$ is a unit, $R/(z)$ has $N(z)$ elements, and the group $U$ of units is finite. Fix a prime element $\pi$ of $R$ that divides $2$, and write $q = N(\pi)$ for the size of the residue field $R/(\pi)$. An element $z$ is odd if $\pi \nmid z$. The semiring $\mathbb{N}$ of positive integers, whose only unit is $1$, is the starting point.

*Definition 2.1 (map).* A map is a triple $(\alpha, k, d)$ with $\alpha$ odd, $k \ge 0$, and a digit table $d$: a function on the odd elements that depends only on the class modulo $\pi^k$, the modulus, and satisfies $\pi^k \mid \alpha z + d(z)$. The digit set is $D = \{d(z) : z \text{ odd}\}$. The map is
$$T(z) = \begin{cases} z/\pi & \text{if } \pi \mid z, \\ \alpha z + d(z) & \text{otherwise.} \end{cases}$$
For odd $z$ with $w = \alpha z + d(z) \ne 0$, the accelerated map is $S(z) = w/\pi^{v(w)}$, where $v(w)$ is the exponent of $\pi$ in $w$, so that $S(z)$ is odd and $v(w) \ge k$. If $w = 0$, $S(z) = 0$ and the orbit ends at $0$.

The congruence $d(z) \equiv -\alpha z \pmod{\pi^k}$ depends only on the class of $z$, and since $\alpha$ is invertible modulo $\pi^k$, the map $z \mapsto -\alpha z$ permutes the odd classes, of which there are $q^{k-1}(q-1)$ for $k \ge 1$. In $\mathbb{Z}$, $3x+1$ is the map $(3, 1, d)$ with $d = 1$, and $7x \pm 1$ is $(7, 2, d)$ with $d(x) = 1$ for $x \equiv 1$ and $d(x) = -1$ for $x \equiv 3 \pmod 4$.

*Lemma 2.2.* Let $k \ge 1$. Every digit is odd, and $D$ has exactly one element in each odd class modulo $\pi^k$.

*Proof.* $d(z) \equiv -\alpha z \pmod \pi$, which is odd because $\pi$ is prime. If $d(z) \equiv d(z') \pmod{\pi^k}$, then $\pi^k$ divides $(\alpha z' + d(z')) - (\alpha z + d(z))$ and $d(z') - d(z)$, hence $\alpha(z' - z)$. Since $\alpha$ is odd and $\pi$ is prime, $\pi^k \mid z' - z$, so $z$ and $z'$ lie in the same class and $d(z) = d(z')$. $\square$

So for $k \ge 1$, $d(z)$ is the element of $D$ in the class of $-\alpha z$, and a map is the same as a choice of one odd element in each odd class (proved; for $\mathbb{Z}[i]$ in Lean, `Expr.d_odd`, `Expr.d_inj`).

*Definition 2.3 (cycles and Collatz maps).* A point $x$ is periodic for a function $f$ if $f^p(x) = x$ for some $p \ge 1$, and its cycle is the set of its iterates. A cycle is trivial if it contains a unit, and a unit cycle if all its elements are units. A map is a Collatz map if every odd $z$ has an iterate under $S$ that is periodic with a trivial cycle.

Cycles are taken literally, as sets of elements. A Collatz map has no cycle without a unit and no divergent orbit. Triviality can be read off $T$ or $S$: a nonzero even element reaches its odd part under $T$, between consecutive odd terms of a $T$-orbit there are only even elements, and units are odd. On $\mathbb{N}$ the definition says that every orbit reaches the cycle through $1$, which for $3x+1$ is the Collatz conjecture.

### 2.2 The window

Let $|z|$ be the complex absolute value, so that $N(z) = |z|$ on $\mathbb{Z}$ and $N(z) = |z|^2$ otherwise. An odd step multiplies $|z|$ by about $|\alpha|$, and at least $k$ divisions by $\pi$ follow. The lower end of the window is elementary.

*Proposition 2.4 (below the window).* If $N(\alpha) < q^k$, every orbit of $T$ is eventually periodic.

*Proof.* Let $\rho = |\alpha|/|\pi|^k$, which is below $1$ because $N(\alpha) < N(\pi)^k$, and let $\delta = \max_{d \in D} |d|$. For odd $z$, $|S(z)| \le \rho|z| + \delta/|\pi|^k$. With $R_0 = \delta/(|\pi|^k(1 - \rho))$, $|S(z)| < |z|$ when $|z| > R_0$, and $|S(z)| \le R_0$ when $|z| \le R_0$. Even steps lower $|z|$. So every orbit enters the disc $|z| \le R_0$ and stays in it, and the disc holds finitely many elements of $R$. $\square$

On $\mathbb{Z}$ this also follows from Leigh's growth formula for divergent trajectories [Leigh 1986, Lemma 3, (61)], since every branch of the map then shrinks. These maps play the part of the $x+1$ problem, which Lagarias calls the solved cousin of $3x+1$ [Lagarias 2016].

If $N(\alpha) > q^k$, an odd step with exactly $k$ divisions multiplies the norm by $N(\alpha)/q^k > 1$, so the map expands in the worst case. Whether it contracts on average is the question that Matthews and Watts answer by a weighted product. Their computation carries over to $R$ with norms in place of absolute values, as follows.

Write the map at modulus $\pi^k$ as a map $T'$ with one branch on each class: $z \mapsto \pi^{k-1}z/\pi^k$, that is $z/\pi$, on the $q^{k-1}$ even classes, and $z \mapsto (\alpha z + d(z))/\pi^k$ on the $q^{k-1}(q-1)$ odd classes. The odd terms of an orbit of $T'$ are the odd terms of the orbit of $T$, and $S$ is $T'$ followed by halvings. On $\mathbb{Z}$, $T'$ is a map $(m_i x - r_i)/2^k$ of Matthews and Watts with $m_i = 2^{k-1}$ on the even classes and $m_i = m$ on the odd ones. It satisfies their condition C, $\gcd(m_i, 2^{ka}) = \gcd(m_i, 2^k)$ for all $a \ge 1$, so their Conjectures 1 and 2 apply with the modulus $2^k$ [Matthews and Watts 1985, pp. 29 to 30]. The weights in their product are the stationary probabilities of a Markov matrix on the classes: the entry for a pair of classes $c$, $c'$ modulo $\pi^k$ is the fraction of the classes modulo $\pi^{2k}$ inside $c$ that $T'$ sends into $c'$.

*Lemma 2.5.* For $T'$ this matrix is doubly stochastic, and every class leads to every other. So the stationary probabilities are uniform, $q^{-k}$ for each class.

*Proof.* Let $c$ be an odd class, $z = z_0 + \pi^k t$ with $t$ running over $R$ modulo $\pi^k$. Then $T'(z) = (\alpha z_0 + d)/\pi^k + \alpha t$, and since $\alpha$ is invertible modulo $\pi^k$, each class $c'$ receives exactly one of the $q^k$ classes modulo $\pi^{2k}$ in $c$: the entry is $q^{-k}$. Let $c$ be even. Then $T'(z) = z/\pi$ is determined modulo $\pi^{2k-1}$, its class modulo $\pi^{k-1}$ is fixed by $c$, and the $q^k$ classes modulo $\pi^{2k}$ in $c$ go in equal numbers to the $q$ classes modulo $\pi^k$ inside that class: the entry is $1/q$ for those $q$ classes and $0$ for the rest. So each column sums to $1$. A class $c'$ receives $q^{-k}$ from each of the $q^{k-1}(q-1)$ odd classes, a total of $(q-1)/q$, and $1/q$ from the one even class $c$ with $c/\pi \equiv c' \pmod{\pi^{k-1}}$, so each row sums to $1$. An odd class leads to every class in one step, and an even class leads to an odd one within $k$ steps. $\square$

On $\mathbb{Z}$ this is our computation; Lemma 3.2 of [Matthews and Watts 1985] gives doubly stochastic matrices only when every $m_i$ is prime to the modulus, which fails on the even classes. With uniform weights the product of the norm ratios is
$$\left(\frac{1}{q}\right)^{1/q}\left(\frac{N(\alpha)}{q^k}\right)^{(q-1)/q},$$
which is below $1$ exactly when $N(\alpha) < q^{k + 1/(q-1)}$ (proved, as a computation of the product). Equivalently, the mean change of $\log_2 N$ per step of $T'$ is negative exactly then, and dividing by the fraction $(q-1)/q$ of odd steps gives the mean change per odd step,
$$\log_2 N(\alpha) - \left(k + \frac{1}{q-1}\right)\log_2 q.$$
That an orbit visits the classes with these frequencies is the conjectural part, here as for $3x+1$.

*Definition 2.6 (window).* A map is in the window if
$$q^k < N(\alpha) < q^{k + 1/(q-1)}.$$

For $q = 2$ this is $2^k < N(\alpha) < 2^{k+1}$. For $3x+1$ it reads $2 < 3 < 4$ at modulus $2$, and for Barina's map $4 < 7 < 8$ at modulus $4$; on $\mathbb{Z}$ the product is $(m/2^{k+1})^{1/2}$, $0.866$ for $3x+1$ and $0.935$ for $7x \pm 1$ (computed). The windows for different $k$ are disjoint, so in a given ring the norm of a multiplier in the window determines its modulus.

On $\mathbb{Z}$, the upper end of the window at every $k$ is the dividing line of Conjectures 1 and 2 of Matthews and Watts for $m > 0$ [Matthews and Watts 1985, p. 30]. Leigh's criterion, Corollary (iii) of his Conjecture 1, is the one-sided form for nonzero multipliers, with $|m_i/d|$ in place of $m_i/d$, and so covers $-m$ as well [Leigh 1986, p. 126]. At $k = 1$ the upper end is Heppner's threshold $d^{d/(d-1)}$ with $d = q$ [Heppner 1978, as annotated in Lagarias 2003, entry 78; Heppner's paper not seen]. The norm form above is the same computation written for $R$. The exponent $1/(q-1)$ differs from $1$ only when $q > 2$, as in $\mathbb{Z}[\omega]$ (Section 5), and we did not find the norm form in print.

### 2.3 Score and drift

*Definition 2.7.* For a map with multiplier norm $N = N(\alpha)$ and modulus norm $M = N(\pi^k) = q^k$, the score is
$$\frac{1}{M} - \frac{1}{N} = \frac{N - M}{NM},$$
and the drift is
$$\log_2 \frac{N}{M} - \frac{\log_2 q}{q-1},$$
the mean change of $\log_2 N(z)$ per odd step from Section 2.2.

Inside the window the score lies in $(0, 1/M - q^{-k-1/(q-1)})$, which for $q = 2$ is $(0, 1/(2M))$, and the drift lies in $(-\log_2 q/(q-1), 0)$, which for $q = 2$ is $(-1, 0)$. Below the window the score is negative. Within one modulus the score and the drift order maps alike, since both increase with $N$. Across moduli the score favours the smaller modulus through the term $1/M$, while the drift ignores the size of the modulus; we compare scores within one ring. For $3x+1$ on $\mathbb{N}$ the score is $1/2 - 1/3 = 0.167$ and the drift $\log_2 3 - 2 = -0.415$ bits per odd step, and for $7x \pm 1$ they are $1/4 - 1/7 = 0.107$ and $\log_2 7 - 3 = -0.193$. We have not searched for prior uses of such a score.

## 3. From $\mathbb{N}$ to $\mathbb{Z}$

### 3.1 What changes

$\mathbb{Z}$ has a second unit, $-1$. The prime $\pi = 2$ and $q = 2$ stay as in $\mathbb{N}$; the half-line becomes a line, and $\kappa = 1$ (Section 1.3). With $-1$ comes a question that is empty in $\mathbb{N}$: whether a map commutes with $-1$, that is, whether $T(-x) = -T(x)$. Digits and multipliers may now be negative, since nothing has to stay positive, and the norm $N(m) = |m|$ places $-m$ in the same window as $m$.

### 3.2 What follows

On $\mathbb{N}$ a Collatz map has to reach $1$. On $\mathbb{Z}$ it has to reach a trivial cycle, a cycle through $1$ or $-1$, and which of them it reaches stops mattering. There are at most two trivial cycles. Two cycles of $S$ that meet are equal, because if $x$ is periodic with period $p$ and $y = S^j(x)$, then $x = S^{j(p-1)}(y)$; so distinct trivial cycles are disjoint, and each contains $1$ or $-1$ (proved; Proposition 6.2 in general). For $7x \pm 1$ they are the fixed points $1$ and $-1$; for $-7x \pm 1$, the loop $1 \leftrightarrow -1$ (computed).

The fixed points of a map on $\mathbb{Z}$ can be read off its digit set.

*Lemma 3.1 (fixed points on $\mathbb{Z}$).* Let $(m, k, d)$ be a map on $\mathbb{Z}$ with $k \ge 1$, let $v \ge k$, $\varepsilon = \pm 1$ and $x$ odd. Then $S(x) = \varepsilon x$ with exponent $v$ exactly when $(\varepsilon 2^v - m)x$ lies in $D$.

*Proof.* If $S(x) = \varepsilon x$ with exponent $v$, then $mx + d(x) = \varepsilon 2^v x$, so $d(x) = (\varepsilon 2^v - m)x$, and $v \ge k$ by construction. Conversely, let $\delta = (\varepsilon 2^v - m)x \in D$. Since $v \ge k$, $\delta \equiv -mx \pmod{2^k}$, so $\delta$ is the element of $D$ in the class of $-mx$, which is $d(x)$ by Lemma 2.2. Then $mx + d(x) = \varepsilon 2^v x$ with $\varepsilon x$ odd, so $S(x) = \varepsilon x$. $\square$

With $\varepsilon = 1$ and $|x| > 1$ this is a fixed point without a unit. With $\varepsilon = -1$ and a map that commutes with $-1$, $\{x, -x\}$ is a cycle, again without a unit if $|x| > 1$. Section 6.6 proves the lemma for any ring.

*Proposition 3.2.* No map with multiplier $3$ at modulus $2$ is a Collatz map on $\mathbb{Z}$.

*Proof.* There is one odd class modulo $2$, so the map has one digit $\delta$, which is odd. Since $(2^2 - 3)\delta = \delta$ lies in $D$, Lemma 3.1 gives $S(\delta) = \delta$, a fixed point without a unit unless $\delta = \pm 1$. The map $3x + 1$ has the cycles $-5 \to -7 \to -5$ (in odd steps, through $-14$, $-20$ and $-10$) and the cycle of $18$ steps through $-17$, and $3x - 1$ has their negatives. $\square$

The fixed point $\delta$ of $3x + \delta$ is the trivial cycle of $3x+1$ scaled by $\delta$: since $3\delta x + \delta = \delta(3x + 1)$ with $\delta$ odd, the orbit of $\delta x$ under $3x + \delta$ is $\delta$ times the orbit of $x$ under $3x+1$. Lagarias separates these scaled cycles of $3x + k$ from the primitive ones [Lagarias 1990; unverified: known through its abstract and Chamberland, Section 6.2], Barina and Maat prove the case $3n + 3^k$ [Barina and Maat 2025; unverified: abstract only], and Crandall proved that $qx + r$ with $r \ge 3$ has an orbit in $\mathbb{N}$ that never reaches $1$ [Crandall 1978, as annotated in Lagarias 2003, entry 46]. The step to $\mathbb{Z}$ is that the digits $\pm 1$ fail as well, through the negative cycles. For $m = -3$, $2^v + 3$ is never $\pm 1$, so Lemma 3.1 with $\varepsilon = 1$ gives nothing; $-3x \pm 1$ have a cycle of $12$ odd steps through $\pm 13$ (computed to $10^6$), and other digits for $-3$ were not tried.

*Proposition 3.3.* No map on $\mathbb{Z}$ at modulus $2$ commutes with $-1$.

*Proof.* Commuting requires $m(-x) + d(-x) = -(mx + d(x))$, that is, $d(-x) = -d(x)$. There is one odd class modulo $2$, so $d(-x) = d(x)$. Hence $d(x) = 0$, which is even, against Lemma 2.2. $\square$

The window forces the multipliers $\pm 3$ to modulus $2$, since $2 < 3 < 4$, so no map in the window with multiplier $\pm 3$ commutes with $-1$. We did not find this one-line observation stated in any source we read. From modulus $4$ on, commuting with $-1$ comes free with the smallest digits.

*Lemma 3.4.* For $k \ge 2$, each odd class modulo $2^k$ has a unique element of least absolute value, and these elements form the set $D_k = \{d \text{ odd} : |d| < 2^{k-1}\}$, which is closed under $x \mapsto -x$. So the map with digit set $D_k$ commutes with $-1$. With $2^k < m < 2^{k+1}$ it has $S(1) = 1$.

*Proof.* The interval $(-2^{k-1}, 2^{k-1})$ has length $2^k$ and even endpoints, so it holds exactly one element of each odd class, and every other element of the class has absolute value above $2^{k-1}$. If $D$ is closed under negation, $-d(x)$ is the element of $D$ in the class of $-m(-x)$, so it is $d(-x)$. For $S(1)$, the digit $d$ of $1$ satisfies $2^k \mid m + d$ and $2^k - 2^{k-1} < m + d < 2^{k+1} + 2^{k-1}$, so $m + d$ is $2^k$ or $2^{k+1}$. $\square$

The precedent is Matthews's map $L_d$, which uses the least absolute residues $-d/2 < i \le d/2$ [Carnielli 2015; Tremblay 2019]. For odd $d$ these are closed under negation and $L_d$ is odd; for even $d$ the tie at $\pm d/2$ is broken toward $+d/2$ and $L_d$ is not. In the maps here the only tie is $\pm 1$ at modulus $2$, which is why $3x+1$ and $3x-1$ both use smallest digits.

At modulus $4$, $D_2 = \{\pm 1\}$, and the two maps in the window are $7x \pm 1$, Barina's map, and $5x \pm 1$, with digit $-1$ for $x \equiv 1$ and $+1$ for $x \equiv 3 \pmod 4$. For $7x \pm 1$ Barina's check to $10^{15}$ [Barina 2018] supersedes our own to $10^7$, which tests our code. The map $5x \pm 1$ has, besides the fixed point $1$, the cycle
$$7 \to 36 \to 18 \to 9 \to 44 \to 22 \to 11 \to 56 \to 28 \to 14 \to 7$$
and its negative (computed). We did not find this map or its cycle in the literature or the OEIS, as far as we searched: OEIS searches for the values of the map ($4, 1, 16, 2, 24, 3, 36, 4, 44, 5, 56$), of its accelerated form ($1, 1, 3, 9, 11, 7, 1, 19, 21$) and for the cycle, without signs, found nothing. Up to sign, the stretch $36, 18, 9, 44, 22, 11, 56, 28, 14, 7$ occurs in del Arte's orbit of $5$ under $-5x+1$ [OEIS A305057], whose terms there are $36, 18, 9, -44, -22, -11, 56, 28, 14, 7$; but $-5x+1$ sends $7$ to $-34$, so the orbit does not close.

Commuting with $-1$ matters for a plain reason. For an odd map $S(-x) = -S(x)$, so the orbits of negative integers are the negatives of those of positive integers, and the problem on $\mathbb{Z}$ is the problem on $\mathbb{N}$, as Barina notes for $7x \pm 1$ [Barina 2018].

The failure of $3x \pm 1$ on $\mathbb{Z}$ is not caused by their failing to commute with $-1$, because maps that do not commute with $-1$ can be Collatz maps on $\mathbb{Z}$. By Lemma 3.4 such a map needs, from modulus $4$ on, a digit that is not the smallest in its class. At modulus $4$ it also needs a negative multiplier: for $m = 5$ and $m = 7$, $2^2 - 5 = -1$ and $2^3 - 7 = 1$, so by Lemma 3.1 with $\varepsilon = 1$ every digit $\delta$ is $(2^v - m)x$ for $x = \mp\delta$, and a Collatz map has only the digits $\pm 1$, which commute (proved). For $m < 0$, $2^v - m$ is never $\pm 1$. Of $80$ random tables at modulus $4$ that do not commute with $-1$, with $|d| \le 15$ and $m = \pm 5, \pm 7$, six are Collatz maps to $10^6$, all with $m < 0$, among them $m = -5$ with digits $13$ and $3$ for the classes $1$ and $3$ modulo $4$, and $m = -7$ with $-3$ and $11$ (computed to $10^6$). A search for maps with the unit loop $1 \leftrightarrow -1$ found $5$ Collatz maps at modulus $4$ ($-7x \pm 1$ and four with $m = -5$, for instance digits $1$ and $11$), $106$ at modulus $8$, and $201$ in a sample of $3000$ at modulus $16$. Positive multipliers occur at modulus $16$, for instance $m = 21$ with digits $17, 3, 37, 23, -7, -37, 13, -1$ for the classes $1, 3, \ldots, 15$ (computed to $10^6$).

For the maps with digits $D_k$, Lemma 3.1 gives the fixed points exactly.

*Proposition 3.5 (fixed points of the smallest-digit maps).* Let $k \ge 2$, $m$ odd with $2^k < m < 2^{k+1}$, and $\delta = \min(m - 2^k, 2^{k+1} - m)$. With digits $D_k$, an odd $x > 0$ is a fixed point exactly when $x < 2^{k-1}/\delta$, and no odd $x$ has $S(x) = -x$. There are $\lfloor (2^{k-1}/\delta + 1)/2 \rfloor$ fixed points up to sign, at least two exactly when $m < \frac{7}{6}2^k$ or $m > \frac{11}{6}2^k$.

*Proof.* By Lemma 3.1, $x$ is fixed with sign $\varepsilon$ and exponent $v \ge k$ exactly when $|(\varepsilon 2^v - m)x| < 2^{k-1}$. For $\varepsilon = -1$, $|{-2^v} - m| > 2^{k-1}$; for $v \ge k+2$, $2^v - m > 2^{k+1}$. The values $v = k$ and $v = k+1$ give $m - 2^k$ and $2^{k+1} - m$, and exactly one of them is below $2^{k-1}$, since $m \ne 3 \cdot 2^{k-1}$. So the condition is $\delta x < 2^{k-1}$, and since $\delta$ is odd, $2^{k-1}/\delta$ is not an odd integer, which gives the count. Two fixed points up to sign means $3\delta < 2^{k-1}$. $\square$

So in every window from modulus $8$ on, the outer sixths, $m/2^k < 7/6$ and $m/2^k > 11/6$, hold no Collatz map with the smallest digits (proved). At modulus $4$ both $5$ and $7$ have $\delta = 1$, so the proposition gives only the fixed point $1$; the second cycle of $5x \pm 1$ is the $3$-cycle through $7$. We did not find this family of results in the sources we read; Barina's map is its case $k = 2$, $m = 7$.

The integer census covers every map with digits $D_k$ at moduli $4$ to $1024$, $1022$ maps (Table 1). Every odd start with $|x| \le 10^6$ was followed to its cycle in exact arithmetic. For each map with a single cycle up to sign, a drop-below-start check showed that every odd $x$ with $3 \le x \le 10^7$ reaches a smaller $|y|$, so with $S(1) = 1$ and $S(-x) = -S(x)$, every odd $|x| \le 10^7$ reaches $\pm 1$: these maps are Collatz maps to that bound (computed to $10^7$). Every other map has at least two cycles, exhibited and checked independently in exact arithmetic for $765$ maps and given by Proposition 3.5 for the rest (computer-assisted).

*Table 1. Maps on $\mathbb{Z}$ with digits $D_k$, moduli $4$ to $1024$.*

| modulus | maps | Collatz maps to $10^7$ | first such $m$ | excluded by Proposition 3.5 |
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

The $251$ Collatz maps have $m/2^k$ in $[1.193, 1.832]$ and drift between $-0.745$ and $-0.127$ bits, nearly filling the range $(\log_2\frac{7}{12}, \log_2\frac{11}{12}) = (-0.778, -0.126)$ that Proposition 3.5 allows. Their share settles near a quarter from modulus $128$ on. Inside the window they are missing near the resonances, where $m^n$ is close to a power of $2$ and the maps have many cycles of $n$ odd steps (Section 6.7): the tenth of the window around $m/2^k = \sqrt 2$ holds none of its $102$ maps, and at modulus $1024$ every $m$ from $1391$ to $1469$ has a cycle of two odd steps, $53$ of them for $m = 1449$, where $1449^2/2^{21} = 1.0012$ (computed to $10^6$). No orbit appeared to diverge at any modulus up to $1024$; orbits reached $2^{1322}$ and returned (computed). The scores run from $0.107$ for $7x \pm 1$ down through $0.0225$ for $25x \pm 1$ and $0.0149$ for $21x \pm 1$ at modulus $16$.

### 3.3 What persists

The window persists. It is the window of $\mathbb{N}$, and it does not see the sign of the multiplier: Leigh's form of the criterion uses $|m_i/d|$ [Leigh 1986], while the conjectures of Matthews and Watts are stated for positive multipliers. A map with $m > 0$ and positive digits sends positive integers to positive integers, so the question of $\mathbb{N}$ can still be asked of it (Section 8).

## 4. From $\mathbb{Z}$ to $\mathbb{Z}[i]$

### 4.1 What changes

The prime above $2$ is $\pi = 1+i$, and $2 = -i(1+i)^2$ ramifies. The residue field still has $q = 2$ elements, so the window is $(2^k, 2^{k+1})$ as in $\mathbb{Z}$, but a division by $2$ is now two divisions by $\pi$. An element $a + bi$ is even exactly when $a + b$ is even, and $(a+bi)/(1+i) = ((a+b) + (b-a)i)/2$. Norms are sums of two squares, and an odd $\alpha = a + bi$ has $a + b$ odd, so $N(\alpha) \equiv 1 \pmod 4$. There are four units, and $\kappa = 2$. The lattice is two-dimensional: the multiples of $(1+i)^k$ form a square grid, turned by $45$ degrees when $k$ is odd. The multiplier has an angle, so $\alpha^n$ turns as well as grows. Complex conjugation is an automorphism, and it moves the prime: $\bar\pi = -i\pi$.

### 4.2 What follows

*The modulus of $3x+1$ disappears.* At modulus $1+i$ the window is $(2, 4)$, so $N(\alpha) = 3$, and $3$ is not a sum of two squares, since squares are $0$ or $1$ modulo $4$. At modulus $1$ the window $(1, 2)$ holds no integer. So no map in the window has modulus $1$ or $1+i$ (proved; in Lean, `prop3_int` and `prop3`, without the step from the window to $N(\alpha) = 3$). A map with one odd class and one digit, the shape of $3x+1$, has no counterpart inside the window. The REU map of Alvarado and his students uses modulus $1+i$ with $N(2+i) = 5$, above the window $(2, 4)$, so its drift is $\log_2 5 - 2 = +0.32$ bits per odd step, which fits the $5.6\%$ of starts that reached a unit [Alvarado 2025].

*No map commutes with the units at the first modulus that has multipliers.* At moduli $(1+i)^k$ with $k \le 2$, $-1 - 1 = -2 = i(1+i)^2$, so $z$ and $-z$ share a class, and commuting would give $d(-z) = d(z) = -d(z)$, so $d(z) = 0$ (proved; in Lean, `prop1`). This is $k \le \kappa$ (Theorem 6.3).

*Collatz maps on the whole ring at $(1+i)^2$.* The modulus $(1+i)^2 = 2i$ is the first with multipliers in the window: the eight associates of $2 \pm i$, of norm $5$. Classes are classes modulo $2$, and the odd classes are those of $1$ ($a$ odd, $b$ even) and of $i$. The class of $1$ has the two smallest elements $\pm 1$ and the class of $i$ has $\pm i$, so there are four tables with the smallest digits. The conjugacy $z \mapsto uz$ carries the map $(\alpha, d)$ to $(\alpha, z \mapsto u^{-1}d(uz))$ and preserves every property studied here, and it makes the four tables one orbit. So there are eight maps up to conjugacy, one for each multiplier, with $D = \{1, i\}$. Five are Collatz maps and three are not:

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

The Collatz maps are the four associates of $2 - i$ and one associate of $2 + i$. For each of the five, every odd $z$ with $1 < N(z) \le 10^7$ falls below its norm, over all four quadrants ($15{,}708{,}048$ starts each, longest stopping time $260$ steps of $T$), and by a soundness theorem that needs no condition on the units (in Lean, `drop_below_start_sound_all`) every orbit with $N(z) \le 10^7$ reaches a unit (computed to $10^7$). The first map is
$$T(z) = \begin{cases} z/(1+i) & a + b \text{ even}, \\ (2-i)z + i & a \text{ odd}, b \text{ even}, \\ (2-i)z + 1 & a \text{ even}, b \text{ odd}. \end{cases}$$
Its steps $(2-i) + i = 2 = (1+i)^2(-i)$, $(2-i)(-i) + 1 = -2i = (1+i)^2(-1)$ and $-(2-i) + i = -2+2i = (1+i)^3$ give the unit cycle $1 \to -i \to -1 \to 1$, and $(2-i)i + 1 = 2 + 2i = (1+i)^3(-i)$ sends $i$ into it, so $i$ is not periodic. The table, the cycle, the fact that $i$ is not periodic, and the reduction of the $10^7$ claim to the run are in Lean (`digit2_correct`, `E2_cycle`, `E2_I_not_periodic`, `E2_sound`). Its drift is $\log_2(5/8) = -0.68$ bits per odd step and its score $1/4 - 1/5 = 0.05$.

The mirror image $2 + i$ of $2 - i$ fails with the same kind of digits, although conjugation preserves norms; it is the multiplier of the REU map, now at the right modulus. Collatz maps without any condition on the units are plentiful here: with each digit chosen among the odd elements of norm at most $5$, all $288$ maps with $\alpha$ an associate of $2 \pm i$ reached cycles for $N(z) \le 10^5$, and $68$ had a single nonzero cycle (computed to $10^5$).

*Abundance at $(1+i)^3$.* The twelve multipliers are the associates of $3$ and $3 \pm 2i$. The four odd classes each hold exactly one unit, since two distinct units differ by an element of norm $2$ or $4$ and a nonzero multiple of $(1+i)^3$ has norm at least $8$. Here $k = 3 > \kappa$, and maps that commute with the units exist (Theorem 6.3); their digit sets have the form $d_1 U$.

*Proposition 4.1.* A map at modulus $(1+i)^3$ in the window that commutes with the units is not a Collatz map, whatever its digit table (proved in Section 6.6; in Lean, `Expr.no_symmetric_rule_below_29`).

Without the condition on the units the picture changes. With each digit among the six smallest elements of its class, $1204$ tables pass a drop-below-start check to $N(z) \le 10^6$ over all quadrants, $305$ maps up to the conjugacy $z \mapsto uz$, and they cover $8$ of the $12$ multipliers: $\pm 3i$ and all associates of $3 \pm 2i$ except $-3+2i$ and $-2+3i$ (computed to $10^6$; one representative per multiplier to $10^7$). For six of the eight multipliers some Collatz table differs from $D = U$ in one class. For $\alpha = 3i$ the table that gives the classes of $1, i, -1, -i$ the digits $2-i, -1, -i, 1$ has the single trivial cycle $1 \to -i \to -1 \to i \to 1$, which can be checked by hand: $3i + (2-i) = 2+2i = (1+i)^3(-i)$, $3i(-i) + 1 = 4 = (1+i)^4(-1)$, $3i(-1) - i = -4i = (1+i)^4 i$, and $3i \cdot i - 1 = -4 = (1+i)^4$.

*Proposition 4.2.* For $\alpha \in \{-3, -3+2i, -2+3i\}$ there is no Collatz map at modulus $(1+i)^3$, whatever the digit table (proved in Section 6.6).

The multiplier $3$ is open. The fixed-point argument of Section 6.6 removes no digit of norm at most $100$ for it, no table with the six smallest digits passes, $6868$ of the $1{,}500{,}625$ tables with digits of norm at most $100$ pass at $N(z) \le 10^4$, and of a random sample of $30$ of those, $11$ still pass at $10^6$ (computed). An impossibility proof would need an argument that every table has some long cycle without a unit.

*Tied smallest digits at even moduli.* At $(1+i)^{2j}$ the multiples of the modulus form the grid $2^j\mathbb{Z}[i]$, whose Voronoi cell is the square $|x|, |y| \le 2^{j-1}$, and its boundary holds odd points: $2^j$ classes have two smallest elements, so there are $2^{2^j}$ tables with the smallest digits, of which $2^{2^{j-2}}$ commute with the units for $j \ge 2$. That is $4$ and $0$ at $(1+i)^2$, $16$ and $2$ at $(1+i)^4$, $256$ and $4$ at $(1+i)^6$ (proved, and counted by computer). At odd moduli from $(1+i)^3$ on the boundary holds no odd point, so the smallest digits are unique and commute with the units (Section 6.7).

*The units on unequal footing.* The map $2 - i$ above has a unit cycle of length $3$, which is not the order of any unit, and the unit $i$ lies off it. At $(1+i)^3$, Collatz maps have a lone $2$-cycle $\{i, -i\}$ ($3 - 2i$ with digits $1-2i, -i, 1, i$), trivial cycles through units and non-units ($-3i$ with digits $2-3i, 1, i, -1$, whose one trivial cycle has ten odd elements), or three trivial cycles ($16$ of the $240$ passing tables for $3i$), and three does not divide four (computed to $10^6$). Sections 6.2 and 6.4 show that none of this can happen for a map that commutes with the units.

*Rotated and mirrored maps go separate ways.* The conjugate multipliers $2 - i$ and $2 + i$ get opposite verdicts at $(1+i)^2$, and so do the associates $3i$ and $3$ at $(1+i)^3$. A step of the mirrored map is the conjugate of a step of the original times $(-i)^v$, where $v$ is the number of divisions, because $\overline{z/(1+i)} = i\bar z/(1+i)$; and a step of the rotated map is a step of the original applied to $uz$ (Section 6.5). A table that does not commute with the units treats $uz$ and $z$ as unrelated inputs, so the relation stops after one step. For maps that commute with the units, rotated and mirrored maps are Collatz maps together (Theorem 6.7).

*Cycle lengths need the angle as well as the size.* A cycle of $n$ odd steps with $V$ divisions in all needs $\alpha^n$ close to $\varepsilon(1+i)^V$ for a unit $\varepsilon$, and for non-real $\alpha$ that is a condition on the angle of $\alpha^n$ as well as on its size (Section 6.7). Where both hold, cycles crowd. At modulus $(1+i)^7$, $(11+10i)^{33}$ is within a relative distance $6.17 \cdot 10^{-4}$ of a unit times $(1+i)^{257}$, and the map with $\alpha = 11 + 10i$ and the smallest digits has $728$ cycles of $33$ odd steps up to units, against a heuristic count of $764$ (computed; [Paper 2] gives the complete lists up to stated lengths).

*Divergence along lines.* Integers in the window showed no divergent orbit. In $\mathbb{Z}[i]$, among the maps with the smallest digits that commute with the units, orbits that appear to diverge occur only for the real multipliers $5$ at modulus $(1+i)^4$ and $9$, $11$ at $(1+i)^6$, and every escaping orbit ends on one of the eight lines
$$L: \quad x = \pm 1, \quad y = \pm 1, \quad x - y = \pm 1, \quad x + y = \pm 1.$$
For every real multiplier tested from $3$ to $15$, $S$ maps $L$ into itself, checked over a full residue period (computer-assisted). On $L$ the map is one-dimensional, and a two-state model of its valuations gives a drift along $L$ of $+0.122$, $+0.059$ and $+0.187$ bits of $|z|$ per odd step for $5$, $9$ and $11$, and negative values for $3$, $7$, $13$ and $15$ (heuristic). The sign agrees with the observed behaviour in every case. In $\mathbb{Z}$ the line is the whole ring, and every map in the window has negative drift. [Paper 2] treats the lines in detail.

## 5. Testing the invariants in other rings

Each step of the climb changed several invariants at once. From $\mathbb{Z}$ to $\mathbb{Z}[i]$ the number of units went from $2$ to $4$, $2$ became ramified, $\kappa$ went from $1$ to $2$, the lattice became a square grid, and an automorphism appeared that moves $\pi$. Sections 3 and 4 suggest which of these control what was found:

1. whether a modulus carries multipliers depends on which odd numbers are norms (Proposition 6.1);
2. the exponent of the window depends on $q$ (Section 2.2);
3. maps that commute with the units exist exactly from $k = \kappa + 1$ (Theorem 6.3), and the smallest digits commute with the units where the lattice gives each class a unique smallest element (Section 6.7);
4. for maps that commute with the units, the number of trivial cycles divides $|U|$ (Proposition 6.2);
5. a map that does not commute with the units is related to its mirror image only when the automorphism fixes $\pi$ (Theorem 6.7).

Other rings serve as tests of these attributions: a ring in which only some of the invariants differ either confirms an attribution or separates two invariants that $\mathbb{Z}[i]$ changed together. The Eisenstein integers are the one such ring with data. Four further rings are proposed, untested.

### 5.1 The Eisenstein integers

Write $z = a + b\omega$ with $\omega^2 = -1 - \omega$, so $N(z) = a^2 - ab + b^2$. The prime $2$ is inert, so $\pi = 2$ and $q = 4$: $z$ is odd when $a$ and $b$ are not both even, and there are three odd classes modulo $2$. There are six units, $\kappa = 1$, the lattice is triangular, and complex conjugation fixes $\pi$. Against $\mathbb{Z}[i]$, the ring changes $q$, the number of units, the way $2$ factors, $\kappa$, the lattice and the action of conjugation on $\pi$, and keeps the dimension and the angle of the multiplier.

*The window with $q = 4$.* The window is $(4^k, 4^{k+1/3})$, because a division by $2$ divides the norm by $4$ and the mean excess of divisions is $1/3$. At modulus $2$ it is $(4, 6.35)$ and holds no norm, since $5$ and $6$ are not of the form $a^2 - ab + b^2$ (Proposition 6.1), so here too the first modulus has no multiplier. At modulus $4$ it is $(16, 25.40)$, with the odd norms $19$, $21$ and $25$ and five multipliers up to units: $5 + 2\omega$ and its conjugate $5 + 3\omega$ (norm $19$), $5 + \omega$ and $5 + 4\omega$ (norm $21$), and $5$. The window $(4^k, 4^{k+1})$, which a reader carrying over the integer form would expect, admits $23$ multipliers up to units at modulus $4$, and all but these five expand on average (computed). This confirms attribution 2: the exponent follows $q$.

*Commuting maps at the first modulus with multipliers.* Since $\kappa = 1$, maps that commute with the units exist from modulus $4$, which is also the first modulus with multipliers. The $12$ odd classes modulo $4$ form two orbits of six under the units, and each class has a unique element of least norm: the units and the six associates of $1 - \omega$ (norm $3$). So the smallest digits give one table, $D = U \cup (1-\omega)U$, and it commutes with the units (proved). In $\mathbb{Z}[i]$ the first modulus with multipliers, $(1+i)^2$, had no commuting maps; $\mathbb{Z}[\omega]$, a two-dimensional ring with complex multipliers, has them at once. So the delay in $\mathbb{Z}[i]$ comes from $\kappa = e = 2$, not from the dimension, which confirms attribution 3; the rings $\mathbb{Z}[\sqrt{-2}]$ and $\mathbb{Z}[(1+\sqrt{-11})/2]$ of Section 5.2 would separate $\kappa$ from the number of units. Up to units and conjugation there are three maps:

| $\alpha$ | $N(\alpha)$ | drift (bits) | cycles up to units, $N(z) \le 10^5$ |
|---|---|---|---|
| $5 + 2\omega$ | 19 | $-0.419$ | 1, the fixed class of $1$ |
| $5 + \omega$ | 21 | $-0.274$ | 3: $1$, $2 + \omega$ (both fixed), and $7 + 3\omega$ (11 odd steps) |
| $5$ | 25 | $-0.023$ | 84 found, 9104 starts unresolved |

*A Collatz map at once.* For $5 + 2\omega$ and $5 + 3\omega$, every odd $z$ with $1 < N(z) \le 10^6$ falls below its norm, in exact arithmetic, so every odd $z$ with $N(z) \le 10^6$ reaches a unit (computed to $10^6$). Read literally, with cycles as sets and all six associates of each multiplier, $12$ of the $30$ maps are Collatz maps at $N(z) \le 10^5$, namely the associates of $5 + 2\omega$ and $5 + 3\omega$ (computed to $10^5$). So $\mathbb{Z}[\omega]$ has a Collatz map that commutes with the units at its first modulus with multipliers, with the smallest multiplier there; its score is $1/16 - 1/19 = 0.0099$. The forced fixed points of Section 6.6 account for the other two maps. For $5 + \omega$, $4 - \alpha = \omega^2$ is a unit, so a Collatz map would need unit digits, and the non-unit digits give fixed points such as $2 + \omega$. For $\alpha = 5$, $4 - 5 = -1$. For $5 + 2\omega$, $4 - \alpha = -1 - 2\omega$ has norm $3$ and lies in $D$, so the class of $1$ is fixed and no other $z$ passes.

*Trivial cycles among six units.* Among the twelve Collatz maps above, the literal trivial cycles are one cycle through all six units (for $-2-5\omega$, $2-3\omega$, $3+5\omega$, $5+3\omega$), two cycles of three units each ($-5-3\omega$, $-3-5\omega$, $-2+3\omega$, $2+5\omega$), three cycles of two units ($-5-2\omega$, $-3+2\omega$), or six fixed units ($3-2\omega$, $5+2\omega$) (computed). The counts $1, 2, 3, 6$ are the divisors of $6$, and the count $3$, impossible in $\mathbb{Z}[i]$ for commuting maps, appears. This confirms attribution 4 with a unit group whose divisors $\mathbb{Z}[i]$ could not show.

*The mirror.* Because conjugation fixes $2$, the mirror of any map in $\mathbb{Z}[\omega]$ is conjugate to it, whatever its table (Theorem 6.7, proved), and the verdicts for $5 + 2\omega$ and $5 + 3\omega$ agree. In $\mathbb{Z}[i]$ the mirror pair $2 \pm i$ split. This is the one test that the data can make of attribution 5, and it is a consequence of a proof; a computation on non-commuting Eisenstein tables would check the code, not the attribution.

*The real multiplier and the second modulus.* For $\alpha = 5$, $59$ of $139$ random odd starts with $|a|, |b| \le 300$ passed norm $2^{400}$, and each ended on a line at distance one from a line of units, $b = 1$ ($33$ starts) or $a - b = 1$ ($26$), in the sector $0 \le b < a$ that holds one associate of each element (computed). So divergence along lines for real multipliers recurs. At modulus $8$ the window $(64, 101.6)$ gives $16$ multipliers up to units, with norms $67$ to $97$. The $48$ odd classes form eight orbits, one of which has two elements of least norm, $2 - 3\omega$ and $2 + 5\omega$, so there are two tables with the smallest digits that commute with the units, conjugate to each other, and $32$ maps. None has a single cycle at $N(z) \le 10^5$. Most have two or three cycles, and $\alpha = 9$ has $42$ with $5829$ starts unresolved; $45$ of $139$ random starts for $\alpha = 9$ passed norm $2^{400}$, all on the same two lines (computed to $10^5$). The tied orbit gives $64$ tables with the smallest digits in all, and in a sample of two random non-commuting tables for each of the $96$ associates, $18$ of $192$ maps are Collatz maps at $N(z) \le 10^5$, all of norm $91$ (computed to $10^5$). As in $\mathbb{Z}[i]$ at even moduli, a tie lets a table that does not commute with the units succeed where the commuting ones fail.

The REU slides mention an Eisenstein analogue that divides by the ramified prime $1 - \omega$, with $q = 3$, without computations [Alvarado 2025]. Our maps divide by the inert prime $2$, and the version with $1-\omega$ is untested.

### 5.2 Rings that would separate the invariants

None of the following has been computed. For each ring we state what it changes against $\mathbb{Z}[i]$ or $\mathbb{Z}[\omega]$ and what the results of Section 6 predict; the predictions that follow from a theorem are marked as such, and the rest are what a computation would test.

*$\mathbb{Z}[\sqrt{-2}]$.* Here $2 = -(\sqrt{-2})^2$ ramifies as in $\mathbb{Z}[i]$, with $\pi = \sqrt{-2}$, $q = 2$ and $e = 2$, but the units are only $\pm 1$. The norm is $a^2 + 2b^2$, and $3 = 1 + 2 \cdot 1$ is a norm, so the window $(2, 4)$ at modulus $\pi$ holds the multipliers $\pm 1 \pm \sqrt{-2}$, and maps of the shape of $3x+1$, with one odd class and one digit, exist inside the window (proved, by Proposition 6.1). This separates the two causes that coincide in $\mathbb{Z}[i]$: the empty modulus $1+i$ there comes from the norms, while the units would not prevent it. With $U = \{\pm 1\}$, $\kappa = v(-2) = 2$, so no map commutes with the units at $\pi$ or $\pi^2$, and commuting maps start at $\pi^3$, as in $\mathbb{Z}[i]$ (proved, by Theorem 6.3). If the delay of commuting Collatz maps in $\mathbb{Z}[i]$ is caused by ramification, it should recur here with two units. Conjugation sends $\pi$ to $-\pi$, so the mirror carries the factor $(-1)^v$ and, for tables that do not commute with $-1$, should lose the relation after one step as in $\mathbb{Z}[i]$. Whether the maps at modulus $\pi$ are Collatz maps on the whole ring is the first question a computation would answer, and the analogue of Proposition 3.2 would be the test: the forced fixed points apply when $\pi^v - \alpha$ is a unit.

*$\mathbb{Z}[(1+\sqrt{-7})/2]$.* Here $2$ splits: $2 = \theta\bar\theta$ with $\theta = (1+\sqrt{-7})/2$, $N(\theta) = 2$, and $\theta$ and $\bar\theta$ are not associates. So $q = 2$, $e = 1$, $U = \{\pm 1\}$ and $\kappa = 1$, and maps dividing by $\theta$ that commute with $-1$ exist from $\theta^2$, as in $\mathbb{Z}$ (proved, by Theorem 6.3). Conjugation does not fix the prime ideal: it sends a map that divides by $\theta$ to one that divides by $\bar\theta$, so there is no mirrored map in the sense of Section 6.5. This ring tests what the mirror relation needs when the automorphism moves the prime to a different prime; we expect a map and its conjugate to be conjugate maps of two different families, one for each prime above $2$.

*$\mathbb{Z}[(1+\sqrt{-11})/2]$.* Here $2$ is inert, $q = 4$, $e = 1$ and $\kappa = 1$, as in $\mathbb{Z}[\omega]$, but $U = \{\pm 1\}$. The window is $(4^k, 4^{k+1/3})$ and commuting maps exist from modulus $4$ (proved, by Theorem 6.3), and a map has at most two trivial cycles (proved, by Proposition 6.2). This ring separates $q$ from the number of units: if a Collatz map that commutes with the units appears at the first modulus with multipliers, as $5 + 2\omega$ does, the early appearance in $\mathbb{Z}[\omega]$ is due to $q$ and $\kappa$ and not to the six units.

*$\mathbb{Z}[\sqrt 2]$.* The unit group is infinite, generated by $-1$ and $1 + \sqrt 2$, and $2 = (\sqrt 2)^2$ ramifies. No map commutes with all units at any modulus, though maps commuting with $\pm 1$ exist from $(\sqrt 2)^3$ (proved, Theorem 6.3). Leigh's map, $z/\sqrt 2$ and $(3z + 1)/\sqrt 2$ at modulus $\sqrt 2$, lives here, with three invariant one-dimensional sets and apparently divergent orbits [Matthews 2010, Example 10.1]. The ring tests two parts of the framework. The norm can be $\pm 1$ on infinitely many elements and no longer bounds size, so the definition of a trivial cycle and Proposition 2.4, whose proof needs finitely many elements of bounded size, have to be restated with both real embeddings; and the window must be read with the absolute norm $|N|$. We expect divergence along invariant lines to be common there, as it is for real multipliers in $\mathbb{Z}[i]$.

## 6. The invariants and what they control

This section states once, for any ring of the kind fixed in Section 2.1, the results that explain what the climb found. Each statement names the invariant that controls it, points back to where the climb found it, and names the ring of Section 5 that tests it. Throughout, $R$ is $\mathbb{Z}$ or the ring of integers of an imaginary quadratic field, $\pi$ a prime element above $2$, $q = N(\pi)$ and $U$ the unit group; Theorem 6.3 is stated more generally.

### 6.1 Which moduli carry multipliers

The invariant is the set of odd numbers that are norms.

*Proposition 6.1.* The modulus $\pi^k$ carries a multiplier in the window exactly when the interval $(q^k, q^{k+1/(q-1)})$ contains the norm of an odd element. In $\mathbb{Z}$ this holds for every $k \ge 1$. In $\mathbb{Z}[i]$ and in $\mathbb{Z}[\omega]$ it holds exactly for $k \ge 2$.

*Proof.* The first statement is Definition 2.6. In $\mathbb{Z}$, $2^k + 1$ is odd and lies in $(2^k, 2^{k+1})$ for $k \ge 1$, and $(1, 2)$ holds no integer. In $\mathbb{Z}[i]$, an odd $a + bi$ has $a + b$ odd, so one of $a$, $b$ is odd and the other even, and the norm is $1 \bmod 4$. The windows $(1, 2)$ and $(2, 4)$ hold no such number. For $k = 2j \ge 2$, $N(2^j + i) = 4^j + 1$ lies in $(4^j, 2 \cdot 4^j)$, and $2^j + i$ is odd. For $k = 2j + 1 \ge 3$, $N((2^j + 1) + 2^j i) = 2 \cdot 4^j + 2^{j+1} + 1$ lies in $(2 \cdot 4^j, 4^{j+1})$, and the element is odd. In $\mathbb{Z}[\omega]$, an odd $a + b\omega$ has $a$, $b$ not both even, and in each of the three cases $a^2 - ab + b^2$ is odd. The window $(1, 4^{1/3})$ holds no integer, and $(4, 4^{4/3}) = (4, 6.35)$ holds the one odd number $5$, which is not a norm: $4(a^2 - ab + b^2) = (2a - b)^2 + 3b^2$, and $20$ is none of $20 - 0$, $20 - 3$, $20 - 12$ plus a square. For $k \ge 2$, $z = 2^k - \omega$ is odd with $N(z) = 4^k + 2^k + 1$, which lies below $4^{k+1/3}$ because $2^k + 1 < (4^{1/3} - 1)4^k$ for $k \ge 2$. $\square$

The climb found this as the missing modulus $1+i$ in $\mathbb{Z}[i]$ (Section 4.2), where $3$ is not a sum of two squares, and the missing modulus $2$ in $\mathbb{Z}[\omega]$ (Section 5.1). The REU map at modulus $1+i$ shows the consequence: the only multipliers there lie outside the window. We did not find this principle stated in the sources we read. $\mathbb{Z}[\sqrt{-2}]$ tests it, since $3 = 1 + 2 \cdot 1$ is a norm there and the modulus $\pi$ carries multipliers although $2$ ramifies (Section 5.2).

### 6.2 Trivial cycles

The invariant is the unit group $U$: its size bounds the number of trivial cycles, and for maps that commute with the units its divisors give the possible numbers.

*Proposition 6.2.* (1) A map has at most $|U|$ trivial cycles. (2) Let $f$ be a function on $R$ with $f(uz) = uf(z)$ for all $u \in U$, for instance $S$ of a map that commutes with the units. If $1$ is not periodic, no unit is, and there are no trivial cycles. If $1$ is periodic with cycle $C_1$, the trivial cycles are the sets $uC_1$, the set $U \cap C_1$ is a subgroup of $U$ and is the stabilizer of $C_1$, and the number of trivial cycles is $|U|/|U \cap C_1|$, a divisor of $|U|$.

*Proof.* Two cycles of a function that meet are equal: if $x$ is periodic with period $p$ and $y = f^j(x)$, then $f^{j(p-1)}(y) = x$. (1) Distinct trivial cycles are disjoint, and each contains a unit. (2) $f^n(u) = uf^n(1)$, so $u$ is periodic exactly when $1$ is, with cycle $uC_1$, and a trivial cycle through $u$ is $uC_1$. If $uC_1 = C_1$ then $u \in C_1$; if $u \in C_1$, then $uC_1$ and $C_1$ are cycles through $u$, so they are equal. $U \cap C_1$ contains $1$ and is closed under products ($u, u' \in C_1$ gives $uu' \in uC_1 = C_1$), so it is a subgroup of the finite group $U$, and the trivial cycles correspond to its cosets. $\square$

For $\mathbb{Z}[i]$ both parts are in Lean (`trivial_cycles_le_four`, `trivial_cycle_count`). The climb found at most two trivial cycles in $\mathbb{Z}$ (Section 3.2), three trivial cycles for maps at $(1+i)^3$ that do not commute with the units (Section 4.2), so part (2) needs its hypothesis, and the counts $1, 2, 3, 6$ among the Collatz maps of $\mathbb{Z}[\omega]$ (Section 5.1), which is the test with a unit group of order $6$. The count is the orbit-stabilizer theorem for the action of $U$ on cycles; we did not find it stated for Collatz maps in the sources we read.

### 6.3 When maps can commute with the units

The invariant is $\kappa$, the largest valuation $v(u - 1)$ over the units $u \ne 1$.

*Theorem 6.3.* Let $R$ be a Dedekind domain, $P$ a nonzero prime ideal with finite residue field, and $U$ a finite group of units, and call $z$ odd if $z \notin P$. Define a map at modulus $P^k$ as in Definition 2.1, with $P^k$ in place of $\pi^k$, and write $\kappa = \max_{u \in U,\, u \ne 1} v_P(u - 1)$. For $k \ge 0$ the following are equivalent:

1. for some odd $\alpha$ there is a map at modulus $P^k$ that commutes with $U$;
2. for every odd $\alpha$ there is such a map;
3. no $u \in U$ with $u \ne 1$ satisfies $u \equiv 1 \pmod{P^k}$, that is, $k > \kappa$;
4. $U$ acts freely on the odd classes modulo $P^k$.

*Proof.* (3) and (4): for odd $z$, $uz \equiv z \pmod{P^k}$ means $(u-1)z \in P^k$, and since $z \notin P$ this holds exactly when $u - 1 \in P^k$. (1) implies (3): if $u \ne 1$ and $u \equiv 1 \pmod{P^k}$, then $uz$ and $z$ share a class, so $d(uz) = d(z)$, while commuting gives $d(uz) = u\,d(z)$; hence $(1-u)d(z) = 0$ and $d(z) = 0$, which is not odd. (4) implies (2): if $U$ is trivial there is nothing to prove. Otherwise $k \ge 1$, the odd classes fall into orbits of exactly $|U|$ classes, and we choose one class $c$ in each orbit, an element $r \in c$ and an element $\delta_c \equiv -\alpha r \pmod{P^k}$, which is odd. Each odd class is $uc$ for exactly one $c$ and one $u$, and $d(z) = u\delta_c$ on the class $uc$ is a digit table with $d(wz) = w\,d(z)$ for $w \in U$. (2) implies (1) trivially. $\square$

Up to the choice inside each class, a table that commutes with the units is a choice of one digit for one class in each orbit. With $-1 \in U$ and $P$ above $2$, $v_P(-2) = e$, so $\kappa \ge e$.

| ring | $U$ | $\kappa$ | first modulus with maps commuting with $U$ |
|---|---|---|---|
| $\mathbb{Z}$ | $\pm 1$ | 1 | $4$ |
| $\mathbb{Z}[i]$ | order 4 | 2 | $(1+i)^3$ |
| $\mathbb{Z}[\omega]$ | order 6 | 1 | $4$ |
| other imaginary quadratic rings | $\pm 1$ | $e$ | $P^2$ if $2$ is unramified, $P^3$ if it ramifies |

In $\mathbb{Z}[\omega]$, $\omega - 1$ and $\omega^2 - 1$ are odd (norm $3$), while $-\omega - 1 = \omega^2$ and $-\omega^2 - 1 = \omega$ are units, so $\kappa = v_2(-2) = 1$. If the unit group is infinite, as in a real quadratic field, then for every $k$ the kernel of $U \to (R/P^k)^\times$ has finite index in $U$ and so contains some $u \ne 1$: no modulus admits a map that commutes with all units. Commuting with $\pm 1$ alone is possible from $k > e$ (proved; Theorem 6.3 and the thresholds are not in Lean).

The climb found the thresholds as Proposition 3.3 and Lemma 3.4 in $\mathbb{Z}$, as the missing commuting maps at $(1+i)^2$ in $\mathbb{Z}[i]$ (Section 4.2), and as commuting maps at the first modulus in $\mathbb{Z}[\omega]$ (Section 5.1), which tests the attribution to $\kappa$ against the dimension. The known odd maps on $\mathbb{Z}$ sit where the theorem allows: Barina's $7x \pm 1$ and Conway's permutation at modulus $4 = 2^2$, where $k = 2 > \kappa = 1$; Collatz's permutation at modulus $3$, where $v_3(-2) = 0$; and Matthews's $L_d$ for odd $d$, where no class prime to $d$ equals its negative. This consistency is our observation. We did not find the theorem in the sources we read. $\mathbb{Z}[\sqrt{-2}]$, $\mathbb{Z}[(1+\sqrt{-7})/2]$ and $\mathbb{Z}[(1+\sqrt{-11})/2]$ would test the thresholds with two units and the three ways $2$ can factor, and $\mathbb{Z}[\sqrt 2]$ the case of an infinite unit group (Section 5.2).

### 6.4 Unit cycles

The invariant is again $U$, now acting on itself by multiplication.

*Theorem 6.4.* Let $f$ be a function on $R$ with $f(uz) = uf(z)$ for all $u \in U$, and $w = f(1)$. If $w$ is a unit, then $f^n(u) = uw^n$ for every unit $u$, every unit lies on a unit cycle, every unit cycle has length equal to the order of $w$, and there are $|U|/\text{ord}(w)$ of them. If $w$ is not a unit, there is no unit cycle.

*Proof.* $f(u) = uf(1) = uw$, and induction gives $f^n(u) = uw^n$, which equals $u$ exactly when $w^n = 1$. If $w$ is not a unit, $N(uw) = N(w) \ne 1$. $\square$

For $\mathbb{Z}[i]$ this is in Lean (`unit_cycles`, `no_unit_cycle`). The climb found its failure without the hypothesis: the map $2 - i$ at $(1+i)^2$ has a unit cycle of length $3$ with $i$ off it (Section 4.2; in Lean, `E2_cycle`, `E2_I_not_periodic`). In $\mathbb{Z}$, $S(1) = 1$ for $m > 0$ with digits $D_k$, giving the fixed points $\pm 1$, and $S(1) = -1$ for $-m$, giving the loop $1 \leftrightarrow -1$. We did not find the statement in the sources we read.

### 6.5 Rotated and mirrored maps

The invariants are the units and the automorphisms of $R$, and how an automorphism moves $\pi$.

Part of this is classical. Conjugating a map by a unit, $z \mapsto u^{-1}S_\alpha(uz)$, gives the map with the same multiplier and the table $z \mapsto u^{-1}d(uz)$. On $\mathbb{Z}$ this is the passage from $3x+1$ on the negative integers to $3x - 1$ (folklore; [OEIS A037084]), and the scaling $x \mapsto kx$ carries $3x+1$ to $3x+k$ on the $2$-adic integers [Bernstein and Lagarias 1996]; on $\mathbb{Z}[i]$ at modulus $1+i$ it is the REU proposition that $z$ reaches a unit under the digit $u_1$ exactly when $u_2z$ does under the digit $u_1u_2$ [Alvarado 2025]. Section 4.2 uses it to reduce tables. The rotated map changes the multiplier instead.

*Definition 6.5.* For a unit $u$, the rotated map of $(\alpha, k, d)$ is $(u\alpha, k, d^u)$ with $d^u(z) = d(uz)$. Let $\sigma$ be an automorphism of $R$ of order at most $2$ with $\sigma(\pi) = \eta\pi$ for a unit $\eta$. The mirrored map is $(\sigma(\alpha), k, d^\sigma)$ with $d^\sigma(z) = \sigma(d(\sigma(z)))$.

Both are maps: $u\alpha z + d(uz) = \alpha(uz) + d(uz)$, and $\sigma(\alpha)z + \sigma(d(\sigma z)) = \sigma(\alpha\sigma(z) + d(\sigma z))$ is a multiple of $\sigma(\pi)^k = \eta^k\pi^k$. On $\mathbb{Z}[i]$, $\sigma$ is complex conjugation and $\eta = -i$, since $\overline{1+i} = -i(1+i)$; on $\mathbb{Z}[\omega]$, $\sigma$ is conjugation and $\eta = 1$; on $\mathbb{Z}$, $\sigma$ is the identity.

*Proposition 6.6 (one step, no condition on the units).* $S_{u\alpha}(z) = S_\alpha(uz)$. For odd $z$ with $w = \alpha\sigma(z) + d(\sigma(z)) \ne 0$ and $v = v(w)$, $S_{\sigma\alpha}(z) = \eta^v\sigma(S_\alpha(\sigma z))$; so $N(S_{\sigma\alpha}(z)) = N(S_\alpha(\sigma z))$.

*Proof.* $u\alpha z + d^u(z) = \alpha(uz) + d(uz)$, and $uz$ is odd. For the mirror, $\sigma(\alpha)z + d^\sigma(z) = \sigma(w)$, and $w = \pi^v y$ with $y = S_\alpha(\sigma z)$ odd, so $\sigma(w) = \pi^v\eta^v\sigma(y)$ with $\eta^v\sigma(y)$ odd. $\square$

*Theorem 6.7.* (1) If the table commutes with the units, the rotated map commutes with them, $S^n_{u\alpha}(z) = u^nS^n_\alpha(z)$, and it is a Collatz map exactly when the original is. (2) If the table commutes with the units, so does the mirrored map, $S^n_{\sigma\alpha}(z) = \eta^{V_n}\sigma(S^n_\alpha(\sigma z))$ with $V_n$ the total exponent of the first $n$ steps from $\sigma(z)$, the norm sequence of $z$ under the mirror is that of $\sigma(z)$ under the original, and the mirror is a Collatz map exactly when the original is. (3) If $\eta = 1$, then $S_{\sigma\alpha} = \sigma \circ S_\alpha \circ \sigma$ for every table, and the mirror is a Collatz map exactly when the original is.

*Proof.* (1) $d^u(wz) = d(uwz) = w\,d^u(z)$, and Proposition 6.6 with $S_\alpha(uz) = uS_\alpha(z)$ gives $S_{u\alpha} = uS_\alpha$, hence the iterates by induction. (2) $d^\sigma(uz) = \sigma(\sigma(u)d(\sigma z)) = u\,d^\sigma(z)$, and induction on $n$ from Proposition 6.6 moves the unit $\eta^v$ of each step out of the later steps. (3) is Proposition 6.6 with $\eta = 1$; $\sigma$ is a bijection that preserves odd elements, units and periodic points. For the Collatz statements in (1) and (2), let $h = S_\alpha$ or $h = \sigma \circ S_\alpha \circ \sigma$; $h$ commutes with the units and is a Collatz map exactly when $S_\alpha$ is. The new map $g$ commutes with the units and satisfies $g(z) = \varepsilon(z)h(z)$ for a unit $\varepsilon(z)$. By induction, $g^n(z) = \delta_n h^n(z)$ with $\delta_n$ a unit, since $g(\delta_n h^n(z)) = \delta_n\varepsilon h^{n+1}(z)$. If $h^n(z) = y$ lies on a trivial cycle with $h^p(y) = y$, then $x = g^n(z)$ satisfies $g^p(x) = \delta' x$ for a unit $\delta'$, hence $g^{p|U|}(x) = x$, and the cycle of $x$ contains a unit multiple of a unit. The argument with $g$ and $h$ exchanged gives the converse. $\square$

For $\mathbb{Z}[i]$ these are in Lean (`Expr.rotate_S`, `Expr.mirror_S`, `rotate_rule`, `mirror_rule`, `trivial_transfer`). The step from $g$ to $h$ is the standard one for maps that commute with a finite group: two such maps that agree up to a group element at each point induce the same map on the orbit space $R/U$, and a point with $g^p(x) = \varepsilon x$ is periodic [Field 2007, Chapter 2; Lamb and Roberts 1998]. The nearest precedent for the mirror is Kucinski's Theorem 3, which relates parts of Joseph's map by $h(x + yi) = y + xi = i\bar z$ [Kucinski 2000]; Joseph's map divides by $2$, which conjugation fixes, so it is a case of part (3), with no unit factor.

The following were not found in the sources we read. A rotation is the conjugation by $u$ followed by multiplication by $u$, which turns the multiplier into $u\alpha$. The mirrored map carries the factor $\eta^v$, which on $\mathbb{Z}[i]$ is $(-i)^v$ from $\sigma(\pi) = -i\pi$; when the table does not commute with the units, the second step of the mirrored orbit is applied to $\eta^v\sigma(S_\alpha(\sigma z))$, which such a table treats as an unrelated input, so the relation fails after one step. The same remark shows that a map that does not commute with the units depends on the choice of the generator $\pi$ of its prime. A map that commutes with the units comes in a family of up to $2|U|$ maps, the orbit of $(\alpha, d)$ under the group generated by the rotations and the mirror, all Collatz maps together; in $\mathbb{Z}$ the family of $mx \pm 1$ is $\{m, -m\}$, with $S_{-m} = -S_m$, and $7x \pm 1$ and $-7x \pm 1$ have the fixed points $\pm 1$ and the loop $1 \leftrightarrow -1$.

The climb found the failure for tables that do not commute with the units: the pair $2 \pm i$ at $(1+i)^2$ and the associates $3$ and $3i$ at $(1+i)^3$ (Section 4.2). $\mathbb{Z}[\omega]$ tests part (3) (Section 5.1). $\mathbb{Z}[\sqrt{-2}]$, with $\eta = -1$, would test the factor with two units, and $\mathbb{Z}[(1+\sqrt{-7})/2]$, where conjugation moves the prime above $2$ to the other one, lies outside the definition (Section 5.2).

### 6.6 Forced fixed points

The invariant is the distance from $\alpha$ to the unit multiples of the powers $\pi^v$: when $\varepsilon\pi^v - \alpha$ is small, digits become fixed points.

*Lemma 6.8 (forced fixed points).* Let $(\alpha, k, d)$ be a map with $k \ge 1$, $v \ge k$, $\varepsilon$ a unit and $z$ odd. Then $S(z) = \varepsilon z$ with exponent $v$ exactly when $\delta = (\varepsilon\pi^v - \alpha)z$ lies in $D$. If moreover $z$ is not a unit, and $\varepsilon = 1$ or the map commutes with the units, then the cycle of $z$ contains no unit and the map is not a Collatz map.

*Proof.* If $\delta \in D$, then since $v \ge k$, $\delta \equiv -\alpha z \pmod{\pi^k}$, so $\delta = d(z)$ by Lemma 2.2, and $\alpha z + \delta = \pi^v(\varepsilon z)$ with $\varepsilon z$ odd. The converse is the definition of $S$. If $\varepsilon = 1$, $z$ is fixed. If the map commutes with the units, $S^n(z) = \varepsilon^n z$. In both cases every element of the cycle has norm $N(z) > 1$. $\square$

*Corollary 6.9.* Let $v \ge k \ge 1$ and $\varepsilon$ a unit with $\eta = \varepsilon\pi^v - \alpha$ a unit, where $\varepsilon = 1$ unless the map commutes with the units. If the map is a Collatz map, every digit is a unit.

*Proof.* For a digit $\delta$, put $z = \eta^{-1}\delta$, which is odd and satisfies $(\varepsilon\pi^v - \alpha)z = \delta$. If $\delta$ were not a unit, neither would $z$ be, and Lemma 6.8 would apply. $\square$

For $\mathbb{Z}[i]$ both are in Lean (`Expr.fixed_point`, `Expr.fixed_point_not_rule`, `Expr.digits_units`). The integer case is Lemma 3.1, and it contains the known scaled cycles of $3x + d$ and Crandall's result for $qx + r$ (Section 3.2); the general lemma, with the unit $\varepsilon$ and the condition that $\varepsilon\pi^v - \alpha$ be a unit, we did not find in the sources we read. The climb found it as Proposition 3.2, the outer sixths of Proposition 3.5, the digits $\pm 1$ forced at modulus $4$ (Section 3.2), and the fixed classes of $5 + \omega$ and $5$ in $\mathbb{Z}[\omega]$ (Section 5.1). It also proves Propositions 4.1 and 4.2.

*Proof of Proposition 4.1.* Let the map commute with the units. With $\pi = 1+i$, $\pi^3 = -2+2i$ and $\pi^4 = -4$, every multiplier in the window is $u\alpha_0$ with $u$ a unit and $\alpha_0 \in \{3, 3+2i, 3-2i\}$. For these, $-\pi^4 - 3 = 1$, $-i\pi^3 - (3+2i) = -1$ and $-\pi^3 - (3-2i) = -1$, so $\varepsilon\pi^v - u\alpha_0 = u(u^{-1}\varepsilon\pi^v - \alpha_0)$ is a unit for a suitable unit $\varepsilon$, and Corollary 6.9 forces every digit to be a unit. The units lie in distinct odd classes modulo $\pi^3$ (Section 4.2), so $D = U$ and $d(z)$ is the unit $e$ with $\pi^3 \mid \alpha z + e$. Write $S_\beta$ for the map with multiplier $\beta$ and $D = U$. Since $\pi^3 \mid u(\alpha_0 z + e)$ and $ue$ is a unit, $ue$ is the digit of $z$ for $u\alpha_0$ (Lemma 2.2), so $u\alpha_0 z + d(z) = u(\alpha_0 z + e)$ and $S_{u\alpha_0} = uS_{\alpha_0}$; also $S_{\alpha_0}(wz) = wS_{\alpha_0}(z)$ for units $w$, because $D = U$ is closed under the units. Now $\alpha_0 = 3$ sends $-2-i \to 1+2i \to 1-2i = i(-2-i)$, $\alpha_0 = 3+2i$ sends $-3 \to 1+4i \to 5-2i \to -3+2i \to 3$, and $\alpha_0 = 3-2i$ sends $-3 \to 4+i \to -5-2i \to 2-3i \to 3i$, with digits that are units. Let $P$ be the set of unit multiples of the listed points. Then $S_{\alpha_0}$ maps $P$ into itself, so $S_{u\alpha_0} = uS_{\alpha_0}$ does as well, and $P$ contains no unit (its norms are $5$; $9, 17, 29, 13$; $9, 17, 29, 13$). So the orbit of $-2-i$, respectively $-3$, never reaches a trivial cycle. $\square$

*Proof of Proposition 4.2.* $\pi^4 + 3 = -1$, $\pi^3 - (-3+2i) = 1$ and $\pi^3 - (-2+3i) = -i$ are units, so Corollary 6.9 with $\varepsilon = 1$, which needs no condition on the units, forces every digit to be a unit. Then $D = U$, which commutes with the units, and Proposition 4.1 applies. $\square$

$\mathbb{Z}[\sqrt{-2}]$ would test the lemma at the modulus $\pi$ that $\mathbb{Z}[i]$ lacks (Section 5.2).

### 6.7 Known results in the form of the ring

Four further facts that the climb uses are known in the integer case or are standard, and we record them in the form that the rings need.

*The window.* The upper end of the window is the criterion of Matthews and Watts, and of Leigh, written with norms (Section 2.2). Its exponent $1/(q-1)$ is tested by $\mathbb{Z}[\omega]$ (Section 5.1).

*Smallest digits and the lattice.* If every odd class modulo $\pi^k$ has a unique element of least norm, the table of these elements commutes with the units, because multiplication by a unit carries a class to a class and preserves norms. Two elements of a class with the least norm are equidistant from $0$ and from a nonzero element of $\pi^kR$, so they lie on the boundary of the Voronoi cell of $\pi^kR$. Least-norm remainders and their ties are standard in the Euclidean algorithm for these rings, and on $\mathbb{Z}$ the precedent is Matthews's $L_d$ (Section 3.2). In $\mathbb{Z}$ the only tie is $\pm 1$ at modulus $2$; in $\mathbb{Z}[i]$ ties occur exactly at the even moduli, with the counts of Section 4.2, which are our own; in $\mathbb{Z}[\omega]$ there is no tie at modulus $4$ and one tied orbit at modulus $8$ (Section 5.1). The invariant is the lattice shape.

*Cycle lengths at best approximations.* Let a cycle have $n$ odd steps $z_{j+1} = (\alpha z_j + d_j)/\pi^{v_j}$, total exponent $V = v_0 + \cdots + v_{n-1}$, and a unit $\varepsilon$ with $S^n(z_0) = \varepsilon z_0$. Unwinding the steps gives
$$z_0(\varepsilon\pi^V - \alpha^n) = \sum_{j=0}^{n-1} d_j\,\alpha^{n-1-j}\,\pi^{v_0 + \cdots + v_{j-1}}.$$
The right side has size about $|\pi|^V$ times a bounded factor, so $|z_0|$ is of order $1/\text{rel}$ with $\text{rel} = |1 - \alpha^n/(\varepsilon\pi^V)|$, and cycles of large elements need $\alpha^n$ close to $\varepsilon\pi^V$. On $\mathbb{Z}$ this is the condition that $V/n$ approximate $\log_2|m|$, and for $3x+1$ the cycle equation and the role of $2^V - 3^n$ are classical [Böhm and Sontacchi 1978; Steiner 1977; Crandall 1978; Eliahou 1993; Simons and de Weger 2005; Lagarias 1990; Belaga and Mignotte 1998; Wegner 2021]. The form for $R$, and the condition that for non-real $\alpha$ the angle of $\alpha^n$ must be close to that of a unit times $\pi^V$, are ours (Section 4.2). On $\mathbb{Z}$ the number of cycles with $n$ odd steps and total exponent $V$ at a near-coincidence follows the heuristic count
$$E(n, V) = \frac{2^{(k-1)n}\binom{V - kn + n - 1}{n-1}}{2n\,|2^V - m^n|},$$
which comes from the $2^{(k-1)n}$ classes of odd $2$-adic integers with given valuations, each holding one solution of $S^n(x) = \varepsilon x$, together with the guess that this solution is an integer with probability $1/|2^V - m^n|$; the factor $2n$ counts each cycle once up to sign and starting point (heuristic; no prior form found). [Paper 2] uses the Gaussian form of the count. On $\mathbb{Z}$ at modulus $128$, $m = 181$ has $181^2 = 0.999786 \cdot 2^{15}$, $293$ cycles of two odd steps against $E = 292.6$; at modulus $512$, $m = 645$ has $85$ cycles of three odd steps against $E = 84.5$ (counts computed to $10^6$). These are the resonances that empty parts of the integer windows (Section 3.2).

*Divergence on invariant lines.* For real multipliers in $\mathbb{Z}[i]$ and $\mathbb{Z}[\omega]$ the escaping orbits end on lines that $S$ maps into themselves, and the drift computed along a line has the sign of the observed behaviour (Sections 4.2 and 5.1). Computing the drift separately on invariant pieces of a map has integer precedents: Matthews and Watts's Example 5 has the even and the odd integers as two invariant sets, with products below and above $1$ [Matthews and Watts 1985, pp. 40 to 41], and Leigh's Example 2 has two positive classes, one with product $\sqrt 3/2 < 1$ [Leigh 1986, pp. 140 to 141]. On $\mathbb{Z}[\sqrt 2]$, Matthews's Example 10.1 has a real multiplier, three invariant one-dimensional sets and apparently divergent orbits [Matthews 2010], and Kionke's tame cones give divergent points for lattice maps with scalar multipliers [Kionke 2017]. The lines and their drift values are ours. $\mathbb{Z}[\sqrt 2]$ would test the mechanism in the ring of Leigh's map (Section 5.2).

In $\mathbb{Z}[i]$ these results do not yet give a Collatz map that commutes with the units: Proposition 4.1 and Corollary 6.9 extend to every multiplier in the window of norm below $29$, and [Paper 2] proves this bound and studies the first such map, with multiplier $5 + 2i$ at modulus $(1+i)^4$.

## 7. Open questions

1. Is any map in the window a Collatz map? Every positive result here is a check to a finite bound, as for $3x+1$: $7x \pm 1$, the $251$ integer maps of Table 1, the five maps at $(1+i)^2$, the $305$ at $(1+i)^3$, and $5 + 2\omega$.
2. Does $\alpha = 3$ admit a Collatz map at modulus $(1+i)^3$? Fixed points rule out no digit of norm at most $100$, so a proof of impossibility would need an argument about longer cycles.
3. Is there a Collatz map on $\mathbb{Z}$ with multiplier $-3$ at modulus $2$? Lemma 3.1 with $\varepsilon = 1$ gives no constraint, the digits $\pm 1$ fail, and other digits were not tried.
4. Can divergence be proved for the real multipliers $5$, $9$, $11$ in $\mathbb{Z}[i]$ or $5$, $9$ in $\mathbb{Z}[\omega]$, by an exact description of the orbits on the invariant lines in place of the equidistribution heuristic?
5. Does the argument of Terras and Everett [Terras 1976; Everett 1977] carry over? The parity pattern of the first $n$ steps depends only on $z$ modulo a power of $\pi$, and we expect finite stopping time on a set of density one for every map below the upper end of the window. It would not exclude divergence, since Kionke's examples show that both can occur together [Kionke 2017].
6. Do the maps with many cycles have finitely many? This needs an effective lower bound for $|\varepsilon\pi^V - \alpha^n|/|\pi|^V$, a linear form in two logarithms for real $\alpha$, or in a logarithm and an angle otherwise, in the manner of Steiner and of Simons and de Weger for $3x+1$ [Steiner 1977; Simons and de Weger 2005].
7. Do the predictions of Section 5.2 hold? The rings are $\mathbb{Z}[\sqrt{-2}]$ (ramified like $\mathbb{Z}[i]$, units $\pm 1$, multipliers at modulus $\pi$), $\mathbb{Z}[(1+\sqrt{-7})/2]$ ($2$ splits), $\mathbb{Z}[(1+\sqrt{-11})/2]$ ($2$ inert, two units) and $\mathbb{Z}[\sqrt 2]$ (infinite unit group, Leigh's map). None has been computed. The Eisenstein maps that divide by $1 - \omega$, with $q = 3$, are also untested.
8. Is there a map whose multiplier or modulus depends on the class, of the type of Matthews [Matthews 2010], that restricts to $3x+1$ on the real line of $\mathbb{Z}[i]$ and has a single cycle? By the argument of Section 8, item 4, it would need modulus at most $(1+i)^2$ on the class of $3$ modulo $4$. A single-cycle example would contain the Collatz conjecture.
9. Has the score $1/M - 1/N$, or an equivalent, been used before? We have not searched.

## 8. Miscellaneous

These items from the working notes bear on the subject but did not fit the thread.

1. History. Collatz says he invented the problem before 1952 and passed it to Hasse in 1952, and Coxeter gave the first published statement in 1971, with a prize of fifty dollars for a proof and one hundred for a counterexample [Lagarias 2016, p. 772; Collatz 1986; Coxeter 1971]. Erdős published nothing on $3x+1$; the remark "hopeless" is reported from conversation, and the "mathematics is not ready" remark is not in Lagarias's account [Lagarias 2016].
2. Conway proved that a generalization of $3x+1$ to residue-class-wise affine maps is undecidable [Conway 1972], so maps of this kind have to be studied one at a time.
3. Positive digits on $\mathbb{Z}$. With the digit set $\{1, 3, \ldots, 2^k - 1\}$ of least positive residues, the one $3x+1$ uses, and $2^k < m < 2^{k+1}$, the fixed points $1$ and $-1$ occur for every $m$; an odd $x > 0$ is fixed exactly when $x < 2^k/(2^{k+1} - m)$, an odd $x < 0$ exactly when $|x| < 2^k/(m - 2^k)$, and no odd $x$ has $S(x) = -x$ (proved, by Lemma 3.1: the condition is $0 < (\varepsilon 2^v - m)x < 2^k$, which fails for $\varepsilon = -1$, and for $\varepsilon = 1$ forces $v = k+1$ when $x > 0$ and $v = k$ when $x < 0$). So $3$ is fixed when $m > \frac{5}{3}2^k$ and $-3$ when $m < \frac{4}{3}2^k$. These maps preserve $\mathbb{N}$. For $m < \frac{5}{3}2^k$ at moduli $4$ to $1024$, run on all odd $|x| \le 10^5$, the number with a single cycle on the positive integers is $0, 1, 2, 3, 5, 15, 33, 55, 112$ by modulus, and the smallest multiplier $2^k + 1$ has a single positive cycle at every modulus from $8$ to $1024$ (computed to $10^5$). Which of them are Collatz maps on all of $\mathbb{Z}$, in the window $\frac{4}{3}2^k < m < \frac{5}{3}2^k$ that the fixed points leave, was not checked.
4. $3x+1$ does not survive in $\mathbb{Z}[i]$ as a restriction. The multiplier $3$ has norm $9$ and falls at modulus $(1+i)^3$. If a map with $\alpha = 3$ takes the odd step $n \mapsto 3n+1$ at every positive odd integer, then $(1+i)^k$ divides $10 = -i(1+i)^2(2+i)(2-i)$, so $k \le 2$ and $9 > 2^{k+1}$: the map is above the window (proved; the step $k \le 2$ in Lean, `prop5_2`, `prop5_2'`). On the real axis the map with $\alpha = 3$ and $D = U$ at $(1+i)^3$ is the accelerated $3x+1$ map applied to whichever of $\pm x$ is $1$ modulo $4$, so every real orbit decreases to $1$; its other cycles lie off the axes. The multiplier $3$ is inert, so multiplication by it preserves the real line, while a split prime such as $2 + i$ leaves no line invariant (proved).
5. Below the window in $\mathbb{Z}[i]$. The map $\alpha = 1+2i$ at $(1+i)^3$ with $D = U$ lowers the norm at every odd step from a non-unit, and the units form one cycle $1 \to -i \to -1 \to i \to 1$ (proved; [Paper 2] gives the details). With $\alpha = 1 \pm 2i$ or $2 \pm i$ at $(1+i)^3$ and $D = U$, every orbit with $N(z) \le 10^5$ reached a unit within $20$ odd steps (computed).
6. Joseph's map on $\mathbb{Z}[i]$ has mean change $\frac{3}{4}\ln 9 - \ln 4 = +0.26$ of $\ln N$ per step, if the classes modulo $2$ are visited evenly. Of $314{,}197$ starts with $N(z) \le 10^5$, $98.2\%$ had not reached a cycle after $20{,}000$ steps or passed coordinates $2^{100}$, and the rest fell into $67$ cycles, not compared with Kucinski's list of $77$ cycles of period at most $400$ [Kucinski 2000; unverified: read through a text extraction] (computed). With $\alpha = 1 \pm 2i$ in place of $3$, all $16$ small choices of the added terms converge on $N(z) \le 10^5$ and $8$ have a single cycle (computed).
7. Matthews and Watts's Example 2, $x/2$ and $(5x-3)/2$, has two ergodic sets when $3$ divides the modulus [Matthews and Watts 1985, p. 40]: the multiples of $3$ are invariant. It is an early instance of Lemma 3.1, since $2^2 - 5 = -1$ and $5 \cdot 3 - 3 = 4 \cdot 3$, which the authors do not remark.
8. Integer conjugacies of a single map: Kohl's `Mirrored` conjugates an rcwa map by $n \mapsto -n-1$ [Kohl 2005], and Monks and Yazinski find an autoconjugacy of $3x+1$ [Monks and Yazinski 2004]. Neither changes the multiplier, so neither is a rotation in the sense of Section 6.5.
9. A check of ours, not reproducible from the repository, found that every $1 \le x < 10^5$ reaches $1$ under Matthews's $L_3$, a map at modulus $3$ that commutes with $-1$.
10. Kohl's residue-class-wise affine maps include maps of $\mathbb{Z}^2$ with $2 \times 2$ integer matrix multipliers, and each step of a Gaussian map is one of them [Kohl 2005; unverified: no worked example on $\mathbb{Z}^2$ seen]. Kionke proposes integer matrix multipliers as an open extension of his lattice maps [Kionke 2017], and multiplication by a Gaussian $\alpha$ is of that kind.
11. Siegel and O'Dwyer prove almost bounded orbits on a set of logarithmic density one for Hydra maps on rings of integers under a dimension constraint [Siegel and O'Dwyer 2026; unverified: the text seen was cut off]. Their heuristic $\rho_H < 1$ gives the upper end of the window, if the operator norm of multiplication by $\alpha$ is $|\alpha|$ (our computation). Nichols proves a theorem of Terras type in function rings over $\mathbb{F}_2$ [Nichols 2016; unverified: abstract only].
12. Garg studies affine rules on Gaussian integers with dyadic denominators, which are not integral on $\mathbb{Z}[i]$ [Garg 2026; unverified: summary only], and Aliyev lists a Gaussian version of rational cycles as future work [Aliyev 2025].
13. Lagarias restates $3x+1$ through the semigroup of the inverse maps $2x$ and $(2x-1)/3$: the integers in the orbit of $4$ form the backward tree [Lagarias 2016; Klarner and Rado 1974]. [Paper 2] carries the restatement over to a Gaussian map.

## 9. Code and data

Everything is in `research/collatz/`. Integer maps: `zcz.c` (census and drop-below-start check, exact $128$-bit and multi-word arithmetic), `zany.c` (any digit table, literal cycles), `zcensus.py`, `zsummary.py`, `zverify.py` (independent check of two cycles per map), `zesc.py`, `zresonance.py` (the count $E$), `zsym.py`. Gaussian maps: `gcz.c` (full classification), `gstop.c` and `gstopall.c` (drop-below-start checks, the second over all quadrants with no condition on units), `m3search.c` and `m3search.py` (the search at $(1+i)^3$), `symtest.py`, `rules.py`, `exact.py` (exact iteration, digits from the congruence), `sweep_gen.py`, `below_window.py`, `joseph.py`, and the line scripts `famlines.py`, `famline1d.py`, `famlinerun.py`. Eisenstein maps: `zeis.py`, `zeisline.py`, `eistest.py`. Logs are in `logs/`, among them `zsummary_sym.txt`, `zcensus_sym_drop_1e7.txt`, `zverify.txt`, `zresonance.txt`, `zcensus_pos_k2-10_B100000_mlt5over3.txt`, `symtest_Z_*`, `symtest_k2_B1e5.txt`, `symtest_k2_gstopall_1e7.txt`, `m3search_*`, `zeis_k2_B1e5.txt`, `zeis_k2_drop.txt`, `zeis_k3_B1e5.txt`, `zeis_lines.txt` and `eistest_*`. The working notes are `integer-census.md`, `symmetry-test.md`, `theorems.md`, `modulus3-search.md` and `score.md`, and the prior-art notes are `invariants-prior-art.md`, `leigh1986.md`, `matthews-watts1985.md` and `rotation-prior-art.md`. The Lean 4 proofs are in `lean/` (core Lean, toolchain v4.35.0, no Mathlib, no `sorry`, `admit` or `native_decide`; the axioms of every main theorem are at most `propext`, `Classical.choice` and `Quot.sound`), built with `lake build` and checked with `lake env lean CheckAxioms.lean`. The Lean development covers $\mathbb{Z}[i]$ only. The products of Section 2.2 and the check of Lemma 2.5 on $\mathbb{Z}[i]$ at $(1+i)^4$ were run from scratchpad scripts recorded in `matthews-watts1985.md`, and the check of $L_3$ in Section 8 from a scratchpad script; none of these is in the repository.

## References

- [Aliyev 2025] Aliyev, *Notes on Number Theory and Discrete Mathematics* 31(3) (2025), 658-666 [unverified: title and initials not recorded in the working notes].
- [Alvarado 2025] A. Alvarado, The Collatz conjecture over the Gaussian integers, talk slides, University of Colima, 2 April 2025, on work of a 2017 REU at CSU Channel Islands (NSF 1359165). https://fefoaranda.com/wp-content/uploads/2025/04/colima-alvarado-2025.pdf [unverified: slides only; the paper, said to be submitted, was not found].
- [Barina 2018] D. Barina, 7x ± 1: Close Relative of the Collatz Problem, arXiv:1807.00908 (2018); *Computational Methods in Science and Technology* 28(4) (2022), 143-147, doi:10.12921/cmst.2022.0000025.
- [Barina and Maat 2025] D. Barina and Maat, arXiv:2602.05732; *Computational Methods in Science and Technology* 31 (2025), 67-70 [unverified: abstract only; title and second author's initials not recorded].
- [Belaga and Mignotte 1998] E. Belaga and M. Mignotte, on cycles of $3x + d$, *Experimental Mathematics* 7 (1998) [unverified: known through Lagarias's annotations; title and pages not recorded].
- [Bernstein and Lagarias 1996] D. J. Bernstein and J. C. Lagarias, The $3x+1$ conjugacy map, *Canadian Journal of Mathematics* 48 (1996), 1154-1169.
- [Böhm and Sontacchi 1978] C. Böhm and G. Sontacchi, on the existence of cycles of given length in $3x+1$ sequences, *Atti della Accademia Nazionale dei Lincei, Rendiconti* 64 (1978) [unverified: known through Lagarias 2003, entry 26; title and pages not recorded].
- [Carnielli 2015] W. Carnielli, Some natural generalizations of the Collatz problem, *Applied Mathematics E-Notes* 15 (2015), 207-215.
- [Chamberland] M. Chamberland, An update on the $3x+1$ problem. https://chamberland.math.grinnell.edu/papers/3x_survey_eng.pdf
- [Collatz 1986] L. Collatz, On the motivation and origin of the $3x+1$ problem (in Chinese), *J. Qufu Normal University, Natural Science Edition* 12(3) (1986), 9-11; English translation in *The Ultimate Challenge: The $3x+1$ Problem*, American Mathematical Society, 2010, 241-247 [unverified: citation copied from Lagarias 2016].
- [Conway 1972] J. H. Conway, Unpredictable iterations, in *Proceedings of the 1972 Number Theory Conference*, University of Colorado, Boulder, 1972, 49-52 [unverified: citation copied from Lagarias 2016].
- [Coxeter 1971] H. S. M. Coxeter, Cyclic sequences and frieze patterns (The Fourth Felix Behrend Memorial Lecture), *Vinculum* 8 (1971), 4-7 [unverified: citation copied from Lagarias 2016].
- [Crandall 1978] R. E. Crandall, On the "$3x+1$" problem, *Mathematics of Computation* 32 (1978), 1281-1292 [unverified: known through Lagarias 2003, entry 46].
- [Eliahou 1993] S. Eliahou, The $3x+1$ problem: new lower bounds on nontrivial cycle lengths, *Discrete Mathematics* 118 (1993) [unverified: from memory and Chamberland's survey; pages not recorded].
- [Everett 1977] C. J. Everett, Iteration of the number-theoretic function $f(2n) = n$, $f(2n+1) = 3n+2$, *Advances in Mathematics* 25 (1977), 42-45 [unverified: from memory].
- [Field 2007] M. Field, *Dynamics and Symmetry*, Imperial College Press, 2007 [Chapters 1 and 2 read in an excerpt].
- [Garg 2026] A. Garg, Collatz-type dynamics on Gaussian integers: fractal stability islands, long cycles, and dyadic rational structure, Zenodo, doi:10.5281/zenodo.19078383 (March 2026) [unverified: summary only].
- [Hasse 1975] H. Hasse, *Unsolved Problems in Elementary Number Theory*, lectures, 1975, pp. 23-33 [unverified: known through Lagarias 2003, entry 76].
- [Heppner 1978] E. Heppner, Eine Bemerkung zum Hasse-Syracuse-Algorithmus, *Archiv der Mathematik* 31 (1978), 317-320, doi:10.1007/BF01226454 [unverified: known through Lagarias 2003, entry 78; the paper was not seen].
- [Joseph 1998] J. A. Joseph, A chaotic extension of the $3x+1$ function to $\mathbb{Z}_2[i]$, *Fibonacci Quarterly* 36(4) (1998), 309-316.
- [Kionke 2017] S. Kionke, A geometric approach to divergent points of higher dimensional Collatz mappings, arXiv:1511.05893 (2015); *Monatshefte für Mathematik* 182 (2017) [unverified: journal details from a secondary source].
- [Klarner and Rado 1974] D. A. Klarner and R. Rado, Arithmetic properties of certain recursively defined sets, *Pacific Journal of Mathematics* 53(2) (1974), 445-463 [unverified: citation copied from Lagarias 2016].
- [Kohl 2005] S. Kohl, *Restklassenweise affine Gruppen*, Dissertation, Universität Stuttgart, 2005; RCWA, a GAP package.
- [Kucinski 2000] G. I. Kucinski, Cycles for the $3x+1$ map on the Gaussian integers, preprint, University of Scranton, May 2000. https://monks.scranton.edu/files/pubs/kucinski.pdf
- [Lagarias 1985] J. C. Lagarias, The $3x+1$ problem and its generalizations, *American Mathematical Monthly* 92 (1985), 3-23.
- [Lagarias 1990] J. C. Lagarias, The set of rational cycles for the $3x+1$ problem, *Acta Arithmetica* 56 (1990), 33-53 [unverified: not read; known through its abstract and Chamberland's survey].
- [Lagarias 2003] J. C. Lagarias, The $3x+1$ problem: an annotated bibliography (1963-1999), arXiv:math/0309224; entry numbers as in version 12 [unverified: entries beyond about 100 not read].
- [Lagarias 2010] J. C. Lagarias, The $3x+1$ problem: an overview, in J. C. Lagarias (ed.), *The Ultimate Challenge: The $3x+1$ Problem*, American Mathematical Society, 2010, 3-29; arXiv:2111.02635.
- [Lagarias 2016] J. C. Lagarias, Erdős, Klarner, and the $3x+1$ problem, *American Mathematical Monthly* 123(8) (2016), 753-776, doi:10.4169/amer.math.monthly.123.08.753.
- [Lamb and Roberts 1998] J. S. W. Lamb and J. A. G. Roberts, Time-reversal symmetry in dynamical systems: a survey, *Physica D* 112 (1998), 1-39 [unverified: reference taken from a bibliography; not read].
- [Leigh 1986] G. M. Leigh, A Markov process underlying the generalized Syracuse algorithm, *Acta Arithmetica* 46 (1986), 125-143.
- [Matthews 2010] K. R. Matthews, Generalized $3x+1$ mappings: Markov chains and ergodic theory, in *The Ultimate Challenge: The $3x+1$ Problem*, American Mathematical Society, 2010, 79-103. http://www.numbertheory.org/PDFS/matthews-final-revised.pdf
- [Matthews and Watts 1984] K. R. Matthews and A. M. Watts, A generalization of Hasse's generalization of the Syracuse algorithm, *Acta Arithmetica* 43 (1984), 167-175 [not read].
- [Matthews and Watts 1985] K. R. Matthews and A. M. Watts, A Markov approach to the generalized Syracuse algorithm, *Acta Arithmetica* 45 (1985), 29-42.
- [Monks and Yazinski 2004] K. G. Monks and J. Yazinski, The autoconjugacy of the $3x+1$ function, *Discrete Mathematics* 275 (2004), 219-236 [unverified: known through Lagarias's second bibliography].
- [Nichols 2016] D. Nichols, Analogues of the $3x+1$ problem in polynomial rings of characteristic $2$, *Experimental Mathematics* (2016), arXiv:1610.02545.
- [OEIS A006368] The amusical permutation of the nonnegative numbers (J. H. Conway), The On-Line Encyclopedia of Integer Sequences, https://oeis.org/A006368.
- [OEIS A037084] Positive integers that do not reach 1 under the 3n-1 map, https://oeis.org/A037084 [unverified: title paraphrased from the notes].
- [OEIS A305057] A. del Arte, -5x + 1 sequence starting at 5, May 2018, https://oeis.org/A305057.
- [OEIS A317640] D. Barina, The 7x+-1 function, 2018, https://oeis.org/A317640.
- [Paper 2] The companion paper, `papers/collatz-5p2i.md` (in preparation).
- [Siegel 2021] M. C. Siegel, Functional equations associated to Collatz-type maps on integer rings of algebraic number fields, arXiv:2111.07882 (withdrawn).
- [Siegel 2024] M. C. Siegel, $(p,q)$-adic analysis and the Collatz conjecture, arXiv:2412.02902.
- [Siegel and O'Dwyer 2026] M. Siegel and R. O'Dwyer, Almost-boundedness of Collatz-type maps on number rings via Archimedean methods, arXiv:2609.33018 (2026).
- [Simons and de Weger 2005] J. Simons and B. de Weger, Theoretical and computational bounds for $m$-cycles of the $3n+1$ problem, *Acta Arithmetica* 117 (2005), 51-70 [unverified: from memory].
- [Steiner 1977] R. P. Steiner, A theorem on the Syracuse problem, *Proceedings of the 7th Manitoba Conference on Numerical Mathematics* (1977), 553-559 [unverified: known through Lagarias 2003].
- [Terras 1976] R. Terras, A stopping time problem on the positive integers, *Acta Arithmetica* 30 (1976), 241-252 [unverified: from memory].
- [Tremblay 2019] R. Tremblay, Generalized $3x+1$ mappings: searching for cycles, arXiv:1907.13302.
- [Wegner 2021] F. Wegner, arXiv:2101.08060 (2021) [unverified: title not recorded].
