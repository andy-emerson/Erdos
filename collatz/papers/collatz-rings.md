# Invariants of Collatz maps across rings of integers

*Draft, 6 November 2026. Code, logs and Lean proofs: `collatz/research/`. Results and their status: `collatz/catalog.md`. Citations are markers; the bibliography is written last, from a review of the finished text.*

## Abstract

*(Placeholder: written last.)*

## 1. Introduction

This introduction assumes elementary number theory but no familiarity with the Collatz literature. It reviews only the parts of the literature that later sections build on, introduces our vocabulary, fixes our notation, and summarizes the new results. Readers already familiar with the literature may skip the review and start where our vocabulary is introduced.

The Collatz map $C \colon \mathbb{N} \to \mathbb{N}$ is

$$C(n) = \begin{cases} n/2, & n \text{ even}, \\ 3n + 1, & n \text{ odd}. \end{cases}$$

The **orbit** of $n$ is the sequence $n, C(n), C^2(n), \ldots$, where $C^k$ is the $k$-fold iterate of $C$, and a **cycle** is an orbit that returns to its starting point. The orbit of $1$ is the cycle $1 \to 4 \to 2 \to 1$. The orbit of $5$ is $5 \to 16 \to 8 \to 4 \to 2 \to 1$, which falls into the orbit of $1$. The question is whether for every $n \in \mathbb{N}$ there is a $k \geq 0$ with $C^k(n) = 1$. Collatz posed the question in the 1930s and conjectured an affirmative answer [Lagarias 1985; Collatz 1986]. The conjecture has been verified for all $n$ below $2^{71} \approx 2.36 \cdot 10^{21}$ [Barina 2025] and remains open.

As $C(5) = 16$ shows, orbits of $C$ can grow. More generally, $2 \mid 3n + 1$ for odd $n$. Therefore, every odd step is followed by at least one halving. However, an odd step and the halving that always follows it increase $n$: $C^2(n) > n$ for every odd $n$. Orbits decrease only because $3n + 1$ is sometimes divisible by a higher power of $2$. For any odd $n$, let $v(n)$ be the largest integer with $2^{v(n)} \mid 3n + 1$. The odd step and the $v(n)$ halvings that follow it together take $n$ to

$$\frac{3n + 1}{2^{v(n)}}.$$

The value of $v(n)$ depends only on the residue of $n$ modulo a power of $2$. For example,

$$v(n) \begin{cases} = 1, & n \equiv 3 \pmod{4}, \\ > 1, & n \equiv 1 \pmod{4}, \end{cases}$$

since $3n + 1 \equiv 2 \pmod 4$ when $n \equiv 3 \pmod 4$, and $4 \mid 3n + 1$ when $n \equiv 1 \pmod 4$. More generally, for $j \geq 1$, let $r_j$ be the odd residue modulo $2^j$ with $2^j \mid 3r_j + 1$. It exists and is unique because $3$ is invertible modulo $2^j$, and $r_2 = 1$, $r_3 = 5$. The odd $n$ with $v(n) \geq j$ are those with $n \equiv r_j \pmod{2^j}$, and modulo $2^{j+1}$ they split into two classes:

$$v(n) \;\begin{cases} = j, & n \equiv r_{j+1} + 2^j \pmod{2^{j+1}}, \\ > j, & n \equiv r_{j+1} \pmod{2^{j+1}}. \end{cases}$$

The case $j = 1$ is the example above. For $j = 2$,

$$v(n) \begin{cases} = 2, & n \equiv 1 \pmod{8}, \\ > 2, & n \equiv 5 \pmod{8}. \end{cases}$$

In this way, each increase of $j$ halves the remaining class. Therefore, $v(n) = j$ for a proportion $2^{-j}$ of odd $n$, and the mean of $v(n)$ is

$$E(v(n)) = 1 \cdot \tfrac{1}{2} + 2 \cdot \tfrac{1}{4} + 3 \cdot \tfrac{1}{8} + \cdots = \sum_{j \geq 1} j\frac{1}{2^{j}} = 2.$$

Each halving is equivalent to removing exactly one binary digit. Therefore, to see whether orbits grow or shrink on average, measure the size of $n$ by its number of binary digits. An integer with $2^k \leq n < 2^{k+1}$ has exactly

$$k + 1 = \lfloor \log_2 n \rfloor + 1$$

binary digits, which differs from $\log_2 n$ by less than $1$. The odd step and its $v(n)$ halvings are $v(n) + 1$ steps of $C$, and

$$
\begin{aligned}
\log_2 C^{v(n)+1}(n) &= \log_2 \frac{3n + 1}{2^{v(n)}} \\
&= \log_2 (3n + 1) - \log_2 2^{v(n)} \\
&= \log_2 n + \log_2\left(3 + \frac{1}{n}\right) - v(n) \\
&\approx \log_2 n + \log_2 3 - v(n),
\end{aligned}
$$

so the odd step and its halvings change the approximate number of binary digits, $\log_2 n$, by about $\log_2 3 - v(n)$, and along an orbit these changes add up. If successive values of $v(n)$ behave like independent samples, the mean change per odd step is

$$E\big(\log_2 3 - v(n)\big) = \log_2 3 - E(v(n)) = \log_2 3 - 2 \approx -0.415 < 0.$$

We call this mean the **drift**. Orbits can grow for a long time, but under this heuristic they shrink on average, at a rate set by the drift.

The drift argument is not a proof. It assumes that successive values of $v(n)$ are independent, and a negative drift describes typical orbits only: it rules out neither cycles other than the one through $1$ nor a sparse set of orbits that grow without bound. The nearby map $C_{-1}$, with $3n - 1$ in place of $3n + 1$, has the same drift as $C$, and yet does not always reach $1$. On $\mathbb{N}$ it has the three cycles

$$\begin{aligned}
&1 \to 2 \to 1, \\
&5 \to 14 \to 7 \to 20 \to 10 \to 5, \\
&17 \to 50 \to 25 \to 74 \to 37 \to \cdots \to 136 \to 68 \to 34 \to 17,
\end{aligned}$$

the last of length $18$. These are conjectured to be its only cycles, with every orbit of $C_{-1}$ falling into one of them [Lagarias 1985]. Whether or not the conjecture holds, no orbit that meets a cycle other than the one through $1$ can reach $1$.

### 1.1 From $\mathbb{N}$ to $\mathbb{Z}$

The formula for $C$ also defines a map on $\mathbb{Z}$. It fixes $0$, since $0/2 = 0$, as does every map considered here, so statements about orbits on $\mathbb{Z}$ concern the nonzero starts. On the negative integers it has the three cycles

$$\begin{aligned}
&{-1} \to -2 \to -1, \\
&{-5} \to -14 \to -7 \to -20 \to -10 \to -5, \\
&{-17} \to -50 \to -25 \to -74 \to -37 \to \cdots \to -136 \to -68 \to -34 \to -17.
\end{aligned}$$

With the signs reversed, these are exactly the cycles of $C_{-1}$ on $\mathbb{N}$. Similarly, the orbit of $-1$ under $C_{-1}$ is the cycle $-1 \to -4 \to -2 \to -1$, which with the signs reversed is the cycle of $C$ on $\mathbb{N}$. The reason is that for odd $n$,

$$C(-n) = 3(-n) + 1 = -(3n - 1) = -C_{-1}(n),$$

and halving commutes with negation, which gives the identity

$$C(-n) = -C_{-1}(n) \quad \text{for all } n \in \mathbb{Z}.$$

Since $C_{-1}(n) \neq C(n)$ for odd $n$, $C(-n) \neq -C(n)$ for odd $n$, and therefore $C$ is not an odd function.

Barina's map is odd [Barina 2018]. It is

$$B(n) = \begin{cases} n/2, & n \text{ even}, \\ 7n + 1, & n \equiv 1 \pmod 4, \\ 7n - 1, & n \equiv 3 \pmod 4. \end{cases}$$

The digits are chosen so that $4$ divides the result: $4 \mid 7n + 1$ when $n \equiv 1 \pmod 4$, and $4 \mid 7n - 1$ when $n \equiv 3 \pmod 4$. Therefore, every odd step is followed by at least two halvings. However, an odd step and the two halvings that always follow it increase $n$: $B^3(n) > n$ for every odd $n$. For odd $n$, let $v_B(n)$ be the largest integer with $2^{v_B(n)} \mid B(n)$. Writing $n = 4m + 1$ gives $B(n) = 4(7m + 2)$, and writing $n = 4m + 3$ gives $B(n) = 4(7m + 5)$. Since $m \mapsto 7m + c$ permutes the residues modulo every power of $2$, the extra exponent $v_B(n) - 2$ has the distribution of $v(n) - 1$ for $C$, so

$$E(v_B(n)) = 2 + \Big(\tfrac{1}{4} + 2 \cdot \tfrac{1}{8} + 3 \cdot \tfrac{1}{16} + \cdots\Big) = 3.$$

By the argument for $C$, the drift of $B$ is

$$E\big(\log_2 7 - v_B(n)\big) = \log_2 7 - E(v_B(n)) = \log_2 7 - 3 \approx -0.193 < 0.$$

Since $-n \equiv 3 \pmod 4$ exactly when $n \equiv 1 \pmod 4$, the digit changes sign with $n$, and $B(-n) = -B(n)$ for every $n$. For example, the orbit of $-5$ is the negative of the orbit of $5$,

$$\begin{aligned}
&5 \to 36 \to 18 \to 9 \to 64 \to 32 \to 16 \to 8 \to 4 \to 2 \to 1, \\
&{-5} \to -36 \to -18 \to -9 \to -64 \to -32 \to -16 \to -8 \to -4 \to -2 \to -1,
\end{aligned}$$

and the conjecture that every nonzero orbit reaches $\pm 1$ is equivalent to the conjecture on $\mathbb{N}$. Barina verified the conjecture on $\mathbb{N}$ below $10^{15}$ [Barina 2022].

The same construction can be applied to the multiplier $3$:

$$C_{\pm 1}(n) = \begin{cases} n/2, & n \text{ even}, \\ 3n + 1, & n \equiv 1 \pmod 4, \\ 3n - 1, & n \equiv 3 \pmod 4. \end{cases}$$

For the same reason as $B$, $C_{\pm 1}$ is odd, and $4$ divides $3n \pm 1$ in each case, so every odd step is followed by at least two halvings. But here the odd step and those two halvings decrease $n$: $C_{\pm 1}^3(n) < n$ for every odd $n > 1$. So every orbit decreases to $1$, and the problem is trivial. In general, if the digits of a map with multiplier $m$ make $2^k$ divide $mn + d$, the odd step and its $k$ guaranteed halvings can increase $n$ only if $m > 2^k$, and the same argument as for $C$ gives the drift $\log_2 m - (k + 1)$, which is negative only if $m < 2^{k+1}$. So the problem is of interest exactly when $2^k < m < 2^{k+1}$ (Section 3). At modulus $4$ the candidates are $5$ and $7$: $5x \pm 1$ has a second cycle, $7 \to 9 \to 11 \to 7$ in its odd terms, and $7x \pm 1$ is Barina's map.

The identity $B(-n) = -B(n)$ has a cleaner reading. Let $\{\pm 1\}$ act on $\mathbb{Z}$ by multiplication, and write $\mathbb{Z}/\{\pm 1\}$ for the set of orbits of this action, not a quotient ring:

$$\mathbb{Z}/\{\pm 1\} = \{\{0\}, \{\pm 1\}, \{\pm 2\}, \{\pm 3\}, \ldots\}.$$

The class $\{0\}$ is fixed by every map considered here, and the remaining classes are the positive integers, each named by its positive member. A map on $\mathbb{Z}$ induces a map on $\mathbb{Z}/\{\pm 1\}$ exactly when it sends $n$ and $-n$ into one class, and $B$ does; on the nonzero classes the induced map is $B$ on $\mathbb{N}$. The map $C$ on $\mathbb{Z}$ induces no map on $\mathbb{Z}/\{\pm 1\}$, since $16$ and $-14$ lie in different classes.

### 1.2 Two kinds of map

The maps considered here have the following form. Fix a prime $\pi$ dividing $2$, a modulus $\pi^k$, a multiplier $\alpha$, and a **digit table** $d$ assigning to each odd residue class modulo $\pi^k$ a digit with $\pi^k \mid \alpha z + d(z)$. The map divides by $\pi$ when it can and otherwise sends $z$ to $\alpha z + d(z)$. On $\mathbb{Z}$, Barina's map is the case $\pi = 2$, $k = 2$, $\alpha = 7$, with $d(1) = 1$ and $d(3) = -1$.

Such a map reads its digit from the residue class of $z$ and is defined on the whole ring. When it also respects associates, it induces a map on the quotient by the units, and we call it a **Barina-type map**.

The map $C$ on $\mathbb{N}$ is of a different kind. A map on $\mathbb{Z}$ that respects the classes $\{n, -n\}$ and induces $C$ on $\mathbb{N}$ must send odd $z$ to $3z + \text{sgn}(z)$, and the digit $\text{sgn}(z)$ is not a function of $z \bmod 2^j$ for any $j$. So no map that reads its digit from a residue class, at any modulus, induces $C$. What $C$ does is choose a representative of each class, the positive one, and apply a formula to it. We call a map defined by a choice of representatives and a formula on them a **Collatz-type map**.

On $\mathbb{Z}$ the distinction is easy to overlook, since the positive integers are the only natural choice of representatives. It becomes visible in larger rings.

### 1.3 The Gaussian integers and other rings

In $\mathbb{Z}[i]$ the units are $\pm 1$ and $\pm i$, the prime above $2$ is $1 + i$, and the norm is $N(a + bi) = a^2 + b^2$. Two elements are **associates** if they differ by a unit factor, and the class of $2 + i$ is

$$\{\, 2 + i,\ -1 + 2i,\ -2 - i,\ 1 - 2i \,\}.$$

The analogue of $\mathbb{N}$ is the set $R/U$ of nonzero elements modulo associates, here with $R = \mathbb{Z}[i]$ and $U = \{\pm 1, \pm i\}$.

A Collatz-type map on $\mathbb{Z}[i]/U$ needs a representative in each class, and two natural choices exist. A geometric choice takes the representative in a fixed sector, for example $-\pi/4 < \arg z \le \pi/4$, which selects $2 + i$ from the class above. An arithmetic choice takes the representative in a fixed residue class, for example $z \equiv 1 \pmod{2 + 2i}$ for odd $z$, which selects $-1 + 2i$. Barina-type maps correspond to arithmetic choices.

The two choices give different first examples. On the sector, the multiplier $2 + i$ of norm $5$, at modulus $(1+i)^2$ with the smallest digits, gives a map under which every class appears to reach the class of $1$ (verified to norm $10^7$). For Barina-type maps, no multiplier of norm below $29$ works inside the growth window (proved), and the first is $5 + 2i$ at modulus $(1+i)^4$ (verified to norm $1.3 \cdot 10^{11}$). Seen from $\mathbb{Z}[i]$, the difference between $3x+1$ and $7x \pm 1$ is the difference between a geometric and an arithmetic choice of representatives.

The constructions make sense in any ring of integers $\mathcal{O}_K$ of a number field, with $\pi$ a prime above $2$. The rings differ in a few quantities: the size $q$ of the residue field $\mathcal{O}_K/\pi$, the unit group, the largest valuation $\kappa = \max_{u \ne 1} v_\pi(u - 1)$, the way $2$ factors, and the number of archimedean places. This paper asks which of these quantities control the behaviour of Collatz-type and Barina-type maps. A quantity that does so in every ring is an invariant of the problem.

### 1.4 Earlier work

Maps that multiply on each residue class and divide go back to Hasse. Heppner showed that almost all iterates of Hasse's maps decrease when $m < d^{d/(d-1)}$ and increase when $m > d^{d/(d-1)}$ [Heppner 1978]; Matthews and Watts conjectured the criterion for a multiplier on each class [Matthews and Watts 1985, Conjectures 1 and 2]; Leigh built the underlying Markov chain for arbitrary nonzero multipliers [Leigh 1986]. Written with norms, their criterion becomes the window $q^k < N(\alpha) < q^{k + 1/(q-1)}$ used here in every ring. Matthews treated such maps on rings of integers, with examples on $\mathbb{Z}[\sqrt 2]$ and $\mathbb{Z}[\sqrt 3]$, and proposed a systematic study [Matthews 2010, Sections 10 and 11]. On $\mathbb{Z}[i]$, Joseph and Kucinski studied an extension of $3x+1$ [Joseph 1998; Kucinski 2000], and Alvarado and a 2017 REU studied the multiplier $2 + i$ with division by $1 + i$ [Alvarado 2025].

Restating the conjecture on a quotient by a symmetry has a precedent. Monks and Yazinski showed that $C$ has exactly one nontrivial autoconjugacy, an involution $\Omega$ of $\mathbb{Z}_2$, and restated the conjecture on $\mathbb{Z}_2/\langle \Omega \rangle$: it holds if and only if the orbit of the class of every positive integer under $[x] \mapsto [C(x)]$ contains $[1]$ [Monks and Yazinski 2004]. We make the same kind of restatement for a different and simpler symmetry, multiplication by the units, which every ring has.

### 1.5 Results and plan

A **trivial cycle** is a cycle containing a unit, and a **unit cycle** one consisting of units. A **Collatz map** is a map under which every nonzero orbit reaches a trivial cycle. Results fall into two groups according to the kind of cycle.

For cycles through the units, the quotient by $U$ does most of the work. A map induces a map on $R/U$ exactly when it respects associates, and then it is a Collatz map on $R$ exactly when the induced map sends every class to $[1]$ and $[1]$ is periodic. For maps of the form above, respecting associates is equivalent to commuting with the units, $T(uz) = uT(z)$. A Collatz map has at most $|U|$ trivial cycles, and if it commutes with the units their number divides $|U|$. Maps commuting with the units exist at modulus $\pi^k$ exactly when $k > \kappa$. Rotating the multiplier by a unit, or applying an automorphism, gives a family of up to $2|U|$ maps that are Collatz maps together; for the four rotations of $5 + 2i$ the numbers of trivial cycles are $2, 1, 4, 1$, as predicted.

For cycles that miss the units, a cycle with $n$ odd steps and $V$ divisions forces $|\varepsilon \pi^V - \alpha^n|$ to be small, for some unit $\varepsilon$. This splits into one condition for each archimedean place: on $\mathbb{Z}$ a size condition and a sign, on $\mathbb{Z}[i]$ a size condition and an angle condition. Combined with a verification bound, the conditions bound the cycle length from below. A non-trivial cycle of $7x \pm 1$ has at least $27{,}855{,}251$ odd steps, from Barina's verification. A non-trivial cycle of the $5 + 2i$ map, up to units, has at least $8696$ odd steps, from our verification to norm $1.3 \cdot 10^{11}$; the size condition alone gives $683$ and the angle condition alone $1224$. Neither map has a non-trivial cycle of Steiner's simplest shape [Steiner 1977], unconditionally.

Part I treats $\mathbb{Z}$: the passage from $\mathbb{N}$, the two kinds of map, rotation, and both kinds of cycle, with proofs for $\mathbb{Z}$. Part II states the general results for any ring of integers. Part III tests them on $\mathbb{Z}[i]$, then on $\mathbb{Z}[\sqrt 2]$, where the unit group is infinite, and on $\mathbb{Z}[(1 + \sqrt{-7})/2]$, where $2$ splits.

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
