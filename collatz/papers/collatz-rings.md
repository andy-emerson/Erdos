# Invariants of Collatz maps across rings of integers

*Draft, 6 November 2026. Code, logs and Lean proofs: `collatz/research/`. Results and their status: `collatz/catalog.md`. Each claim carries its status: proved (on paper), Lean (proved in Lean 4), computed to a stated bound, computer-assisted, heuristic, literature, or open. Citations are markers; the bibliography is written last, from a review of the finished text.*

## Abstract

[Placeholder: written last.]

## 1. Introduction

### 1.1 The lineage

The $3x+1$ map sends an even positive integer $x$ to $x/2$ and an odd one to $3x+1$. The Collatz conjecture says that every positive integer reaches $1$ [Lagarias 1985; Lagarias 2010]. The same formula makes sense on $\mathbb{Z}$, where it has more cycles: besides $0$ and $1 \to 4 \to 2 \to 1$ there are cycles through $-1$, $-5$ and $-17$, and these are conjectured to be all [Lagarias 1985]. Since $-(3x+1) = 3(-x) - 1$, the map acts on the negative integers as $3x - 1$ acts on the positive ones.

Generalized maps on $\mathbb{Z}$ multiply by a fixed number on each residue class, add a digit that makes the result divisible by the modulus, and divide. Heppner showed that for Hasse's maps almost all iterates decrease when the multiplier $m$ and the divisor $d$ satisfy $m < d^{d/(d-1)}$ and increase when $m > d^{d/(d-1)}$ [Heppner 1978]. Matthews and Watts conjectured the corresponding growth criterion for maps with a multiplier on each class, from a weighted product of the ratios $m_i/d$ [Matthews and Watts 1985, Conjectures 1 and 2], and Leigh built the Markov chain on residue classes behind the weights, for any nonzero multipliers [Leigh 1986]. Their criterion, written with norms, is the window used in every ring of this paper.

Barina's map $7x \pm 1$ divides an even integer by $2$ and sends $n$ to $7n + 1$ if $n \equiv 1 \pmod 4$ and to $7n - 1$ if $n \equiv 3 \pmod 4$. Barina conjectured that every positive orbit reaches $1$, verified it to $10^{15}$, and noted that the orbits of negative integers are the negatives of those of positive ones [Barina 2018]. So for $7x \pm 1$ the problem on $\mathbb{Z}$ is the problem on $\mathbb{N}$, which is not true of $3x+1$.

Rings other than $\mathbb{Z}$ appear in a few lines of work. Matthews treats generalized maps on rings of integers of number fields, with examples on $\mathbb{Z}[\sqrt 2]$ and $\mathbb{Z}[\sqrt 3]$, and remarks that these rings invite a systematic investigation [Matthews 2010, Sections 10 and 11]; the first such example is attributed to Leigh [Siegel 2024, Section 1.1.2]. On the Gaussian integers, Joseph extended $3x+1$ to $\mathbb{Z}_2[i]$ [Joseph 1998], Kucinski found cycles of that map and symmetries between them [Kucinski 2000], and Alvarado and a 2017 REU studied a map that divides by $1+i$ and otherwise multiplies by $2+i$ and adds a unit [Alvarado 2025]. Kionke and Siegel give general frameworks on lattices and on rings of integers [Kionke 2017; Siegel 2024].

Restating the conjecture on a quotient by a symmetry is not new. Monks and Yazinski showed that the $3x+1$ map $T$ has exactly one nontrivial autoconjugacy, an involution $\Omega$ of the $2$-adic integers that complements parity vectors, and restated the conjecture on the quotient by $\Omega$: it holds if and only if the orbit of the class of every positive integer under the induced map $[x] \mapsto [T(x)]$ contains the class of $1$ [Monks and Yazinski 2004; Monks, survey slides]. We make the same kind of restatement for a different and simpler symmetry, multiplication by the units, which every ring has.

### 1.2 The question and the thread

This paper is a search for invariants. Each ring has its own values of a few quantities: the size of the residue field, the number of units, how close a unit other than $1$ comes to $1$, how $2$ factors, the shape of the lattice, and its automorphisms. When the behaviour of Collatz maps changes from one ring to another, the quantities that changed are the candidates for the cause. We ask which quantities keep controlling the maps as the ring grows, what stops mattering, and what becomes important, including questions that make no sense in $\mathbb{N}$.

The thread is one observation. The positive integers are the nonzero integers with $x$ and $-x$ identified:

$$\mathbb{N} = (\mathbb{Z} \setminus \{0\})/\{\pm 1\}.$$

In any ring $R$ with unit group $U$, the corresponding object is $R/U$, the nonzero elements up to associates, where two elements are associates if one is a unit times the other. In $\mathbb{Z}[i]$ the class of $z$ is $\{z, iz, -z, -iz\}$. A problem posed on $\mathbb{N}$ can be posed on $R/U$, and a map on $R$ gives a map on $R/U$ exactly when it sends associates to associates. Studying Collatz maps this way does not depend on the ring, and the rest of the paper follows from doing so.

The first consequence is a distinction between two kinds of map. A **Collatz-type map** applies a formula to one chosen representative of each class: $3x+1$ applies its formula to the positive representative. A **Barina-type map** reads its digit from the residue class of the element itself, on all of $R$, and respects associates: $7x \pm 1$ is one. On $\mathbb{Z}$ the difference is easy to miss, because there is one natural choice of representatives, the positive integers. It is a difference in kind. No map that reads its digit from a residue class, at any modulus, induces $3x+1$ on $\mathbb{N}$; $3x+1$ is the quotient of $3z + \text{sgn}(z)$, whose digit depends on the sign, while $7x \pm 1$ induces itself. In $\mathbb{Z}[i]$ the representatives can be chosen geometrically, by angle, or arithmetically, by residue class, and the two choices give different first Collatz maps: $2+i$ of norm $5$ on a cone of angles, and $5+2i$ of norm $29$ on the residue-class choice. The distinction that is visible there explains, looking back, why $3x+1$ and $7x \pm 1$ behave differently on $\mathbb{Z}$.

The paper follows the path of an observation, a general statement and a test. Part I takes the step from $\mathbb{N}$ to $\mathbb{Z}$ and records what it shows, with proofs for $\mathbb{Z}$ only. In $\mathbb{Z}$ some of these facts look more complicated than they need to be; their value appears in Part II, which states them once for any ring of integers. Part III tests the general statements: on $\mathbb{Z}[i]$, the running example of a ring larger than $\mathbb{Z}$, and then in two rings that differ from $\mathbb{Z}[i]$ in the ways the statements care about, $\mathbb{Z}[\sqrt 2]$ for the statements about units and a ring in which $2$ splits for the statements about cycle lengths.

### 1.3 What we find

Cycles split into two kinds. A cycle that contains a unit is a **trivial cycle**; a cycle made only of units is a **unit cycle**. A **Collatz map** is a map in which every nonzero orbit reaches a trivial cycle. The results about trivial and unit cycles, and the results about cycles that miss the units, use different tools.

Results on cycles through the units, all proved and most in Lean:

1. A map induces a map on $R/U$ exactly when it respects associates, and then it is a Collatz map on $R$ exactly when the induced map sends every class to the class of $1$ and that class is periodic (transfer). For maps that read their digits from residue classes, respecting associates is the same as commuting with the units.
2. A map has at most $|U|$ trivial cycles. For a Collatz map that commutes with the units their number divides $|U|$, and every unit lies on one.
3. Every map that commutes with the units comes in a family of up to $2|U|$ rotated and mirrored maps that are Collatz maps together and induce the same map on $R/U$ up to conjugation. The number of trivial cycles changes across the family in a way the theory predicts: $2, 1, 4, 1$ for the four rotations of $5+2i$.
4. Whether a variant of a map has a unit cycle, and whether a rotation has a single trivial cycle through all the units, is decided by the class of $1$ on $R/U$.
5. Maps that commute with the units exist at modulus $\pi^k$ exactly when $k > \kappa$, where $\kappa$ measures how close the units come to $1$. In $\mathbb{Z}[i]$, $\kappa = 2$.

Results on cycles that miss the units. A cycle that contains no unit must satisfy a cycle equation, and the equation imposes one condition for each way of measuring size in the ring: size and sign in $\mathbb{Z}$, size and angle in $\mathbb{Z}[i]$. Combined with a verification that every start up to a bound reaches a unit, the conditions give lower bounds on the length of such a cycle.

1. A non-trivial cycle of $7x \pm 1$ has at least $27{,}855{,}251$ odd steps (proved, from Barina's verification to $10^{15}$). We know of no earlier bound for this map.
2. A non-trivial cycle of the $5+2i$ map, up to units, has at least $2190$ odd steps (proved, from our verification to norm $7.6 \cdot 10^9$). The size condition alone gives $169$ and the angle condition alone $161$; together they give more than ten times either. [To update after the run to norm $1.3 \cdot 10^{11}$.]
3. Neither map has a non-trivial cycle of the simplest shape, the analogue of Steiner's $1$-cycles for $3x+1$ [Steiner 1977]. This needs no verification.

The general statements hold where the rings are least like $\mathbb{Z}[i]$. In $\mathbb{Z}[\sqrt 2]$, with infinitely many units, a map can reach the units and then walk through them forever; the theorems about units still hold, with walks in place of cycles. In the ring where $2$ splits, the multiplier $5$ has no angle of its own, and the angle condition still decides its cycles.

We also report the profile of each map we study: how far it is verified, the cycle-length bound, and the expected length of its trajectories from its drift.

### 1.4 Terms

These are the paper's terms in brief, with the invariants in plain words. Section 6 gives the formal definitions.

- A **unit** is an element with a multiplicative inverse. The units of $\mathbb{Z}$ are $\pm 1$ and those of $\mathbb{Z}[i]$ are $\pm 1, \pm i$. In $\mathbb{N}$ the only unit is $1$. Two elements are **associates** if one is a unit times the other, and $R/U$ is the set of nonzero elements up to associates.
- The **norm** $N(z)$ measures size: $N(x) = |x|$ on $\mathbb{Z}$ and $N(a + bi) = a^2 + b^2$ on $\mathbb{Z}[i]$. It is multiplicative, equals $1$ exactly on the units, and counts the classes modulo $z$.
- The **prime above $2$** is a prime $\pi$ dividing $2$: $\pi = 2$ in $\mathbb{Z}$ and $\pi = 1+i$ in $\mathbb{Z}[i]$. An element is **odd** if $\pi$ does not divide it.
- The **residue field size** $q = N(\pi)$ is the number of classes modulo $\pi$. Dividing by $\pi$ divides the norm by $q$.
- **How $2$ factors.** In $\mathbb{Z}[i]$, $2 = -i(1+i)^2$: $2$ is **ramified**. In some rings $2$ stays prime (**inert**), and in others it is a product of two different primes (it **splits**).
- **How close the units come to $1$.** For a unit $u \neq 1$, the exponent of $\pi$ in $u - 1$ says how fine the classes modulo powers of $\pi$ must be before $u$ and $1$ fall into different classes. The largest of these exponents is $\kappa$: $1$ in $\mathbb{Z}$, $2$ in $\mathbb{Z}[i]$.
- **Ways of measuring size.** A ring of integers sits inside the real or complex numbers in one or more ways, its **archimedean places**. $\mathbb{Z}$ and $\mathbb{Z}[i]$ have one; $\mathbb{Z}[\sqrt 2]$ has two, since $a + b\sqrt 2$ has the two sizes $|a + b\sqrt 2|$ and $|a - b\sqrt 2|$. By Dirichlet's unit theorem the unit group is finite exactly when there is one place.
- An **automorphism** is a bijection of the ring that preserves sums and products. On $\mathbb{Z}[i]$ complex conjugation is one.
- A **map** is a multiplier $\alpha$, a modulus $\pi^k$ and a **digit table**, which gives the number added after multiplying, for each odd class modulo $\pi^k$. The map $T$ divides by $\pi$ when it can and otherwise multiplies by $\alpha$ and adds the digit; the accelerated map $S$ does one multiplication and all the divisions that follow.
- A **Collatz-type map** is a formula applied to one chosen representative of each class of $R/U$. A **Barina-type map** reads its digit from the residue class of the element, on all of $R$, and respects associates. A Collatz map can be of either type.
- A map **respects associates** if $T(uz)$ is an associate of $T(z)$, and **commutes with the units** if $T(uz) = uT(z)$, for every unit $u$. In $\mathbb{Z}$ commuting says that $T$ is an odd function.
- A **trivial cycle** contains a unit, a **unit cycle** contains only units, and a **Collatz map** is one in which every nonzero orbit reaches a trivial cycle.
- **Rotated** maps replace $\alpha$ by $u\alpha$ for a unit $u$; the **mirrored** map applies an automorphism such as conjugation to $\alpha$ and the digits.
- The **window** is the range of multipliers for which a map expands in the worst case and contracts on average. The **drift** is the mean change of $\log_2 N$ per odd step if an orbit visits residue classes as a random element would.

# Part I. From N to Z

## 2. N as Z with x and −x identified

[Placeholder: gluing $x$ to $-x$; when a map on $\mathbb{Z}$ gives a map on $\mathbb{N}$; transfer on $\mathbb{Z}$.]

## 3. Two kinds of map

[Placeholder: $3x+1$ as the canonical Collatz-type map, induced by no residue-class map; $7x \pm 1$ as a Barina-type map inducing itself on $\mathbb{N}$; the window with credit to Matthews and Watts, Heppner and Leigh; drift; forced fixed points ($4 - 3 = 1$).]

## 4. Rotation on Z

[Placeholder: $7x \pm 1$ against $-7x \pm 1$, fixed points against the unit loop $1 \leftrightarrow -1$; mirroring does nothing on $\mathbb{Z}$.]

## 5. Cycles on Z

[Placeholder: trivial and unit cycles, at most two; $3x - 1$ with a unit cycle and other cycles; cycles that miss the units, size and sign conditions, the bound for $7x \pm 1$, averaging, no $1$-cycles; the profile of $7x \pm 1$.]

# Part II. The general statements

## 6. Maps over a ring of integers

[Placeholder: setting and definitions; Collatz-type and Barina-type maps; geometric and residue-class representatives; the definition of a Collatz map.]

## 7. The quotient R/U

[Placeholder: descent, transfer, respects associates iff commutes, lifting cycles.]

## 8. Trivial and unit cycles

[Placeholder: at most $|U|$, divisor form; the unit-cycle theorem; when a variant has a unit cycle; when a rotation has a single trivial cycle.]

## 9. Rotation and mirror

[Placeholder: families of up to $2|U|$ maps; what is the same on $R/U$ and what differs on $R$; automorphisms.]

## 10. Which maps exist

[Placeholder: the threshold $\kappa$; moduli that carry multipliers; the window per archimedean place; below the window; forced fixed points.]

## 11. Cycles that miss the units

[Placeholder: one condition per archimedean place; why the joint bound exceeds the single ones; averaging; $1$-cycles.]

# Part III. The general statements tested

## 12. The Gaussian integers

[Placeholder: $\kappa = 2$ and the first commuting maps at $(1+i)^3$; geometric against residue-class representatives, $2+i$ at norm $5$ and $5+2i$ at norm $29$, and what this says back about $3x+1$ and $7x \pm 1$; the $5+2i$ family and its trivial-cycle counts; trivial cycles divide $4$; the joint bound and $1$-cycles for $5+2i$; profiles of $5+2i$ and $2+i$.]

## 13. Infinitely many units: Z[√2]

[Placeholder: unit walks; the window in each embedding; transfer; residue-class maps send only finitely many units to units.]

## 14. A multiplier without an angle: the split ring

[Placeholder, kept on probation: in $\mathbb{Z}[(1+\sqrt{-7})/2]$ the angle condition decides the cycles of the multiplier $5$.]

## 15. Formal proofs

[Placeholder.]

## 16. Open questions

[Placeholder.]

## References

[Placeholder: written last, from a review of the finished text.]
