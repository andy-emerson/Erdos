# Ledger of results

What each paper has proven, verified by computation, conjectured and left open, with the relationships found along the way. Each entry names the section of the paper that holds it. A result moves from one list to another only when the paper changes.

## 1. Gaussian Collatz ([`gaussian-collatz.md`](gaussian-collatz.md))

### 1.1 Proven

1. No rule at level 1 or 2 commutes with units, since $z \equiv -z \pmod{(1+i)^2}$ (Proposition 1).
2. A rule with $N(\alpha) < 2^k$ has every orbit eventually periodic (Proposition 2).
3. No Collatz-like rule exists at level 1, because 3 is not a sum of two squares (Proposition 3). Level 0 has no Collatz-like multiplier either (5.1).
4. At level 2 no rule commutes with units even in the weak sense that $S(-z)$ is an associate of $S(z)$ (5.1).
5. Among minimal rules that commute with units at levels 3 to 5, every rule except $H$ and its mirror image has at least two cycles, shown by exhibiting them (Proposition 4). So levels 3 and 5 have no single-cycle rule of this kind, and at level 4 only $H$ can have one.
6. No rule with $\alpha = 3$ at one level both agrees with $3x+1$ on the positive integers and contracts on average (Proposition 5).
7. No integer rule with multiplier 3 commutes with $-1$ (5.1).

### 1.2 Verified by computation

1. Every orbit of $H$ with $N(z) \le 3 \cdot 10^9$ reaches $\pm 1$ or $\pm i$ (Section 12). This covers all starting points, not only primary ones.
2. $G_3$ and $G_{3+2i}$ have 5 and 4 cycles up to units, with no others to $N(z) \le 10^9$ (Section 6, 5.1).
3. At level 6, 22 of the 112 minimal rules have a single cycle up to units to $N(z) \le 10^6$ (5.1).

### 1.3 Conjectured

1. $H$ has a single cycle up to units (Conjecture 1). This is the main conjecture, and it is of Collatz difficulty.
2. The 22 level-6 rules each have a single cycle. These are unnamed in the paper.
3. The rules with $\alpha = 5$ at level 4, and with $\alpha = 9$ and $11$ at level 6, have divergent orbits (Section 8, open problem 2).

### 1.4 Relationships found

1. A Collatz-like rule needs $2^k < N(\alpha) < 2^{k+1}$; ordinary $3x+1$ is the case $2 < 3 < 4$ at level 1 (Section 3).
2. The direct analogue of $3x+1$ fails in $\mathbb{Z}[i]$ for the same reason 3 stays prime there (Proposition 3).
3. The Gaussian 3 has norm 9, so its natural level is 3, where no symmetric rule has a single cycle. Collatz's single cycle does not survive at its natural Gaussian level (5.1).
4. Level 4 is the first level with a symmetric single-cycle rule, and $H$ is the only one there up to conjugation (5.1).
5. $3x+1$ is single-cycle only on the positive integers; on the negatives it has three further cycles that are not mirror images of $1 \to 4 \to 2$. $H$ has no such half: its two cycles are one loop in four directions, forced by symmetry (5.1).

### 1.5 Open

1. A proof or disproof of Conjecture 1, possibly helped by the angle condition on cycles, which has no analogue in $\mathbb{Z}$ (open problem 1).
2. Divergence proofs for $\alpha = 5$ at level 4 (open problem 2).
3. A density-one stopping-time theorem in $(1+i)$-adic form (open problem 3).
4. $H$ on the 2-adic Gaussian integers (open problem 4).
5. A check of $H$ to $10^{11}$, about nine core-hours (open problem 5).
6. A rule with class-dependent multipliers or levels that restricts to $3x+1$ on the real line (open problem 6).

## 2. Gaussian Goldbach ([`gaussian-goldbach.md`](gaussian-goldbach.md))

### 2.1 Verified by computation

1. Holben and Jordan's D, E (at most $45^\circ$) and F, and Knill's Q, hold for every even $z$ with $N(z) \le 10^{10}$, about $1.57 \cdot 10^{10}$ targets (Section 5).
2. E with a strict inequality fails only at $2$ and $3+i$ and their images (2.2).
3. The hardest cases: E needs $N(p_1) = 1301$, F needs $1949$, and Q's hardest cases all lie on the row $b = 2$ (5.1).

### 2.2 Conjectured

1. D, E, F and Q, from Holben and Jordan (1968) and Knill (2016). None is ours.

### 2.3 Relationships found

1. On the row $b = 2$, Q becomes a Goldbach problem for primes of the form $n^2 + 1$ (5.1).
2. The minimal first prime grows slowly, roughly like $(\ln N)^2$ times a slowly rising factor (5.2).

### 2.4 Open

1. Larger bounds: about 20 core-hours to $10^{12}$ after the sieve-margin fix, which needs a machine (7.1).
2. A second, independent program to certify the run (7.2).

## 3. Weilert's gcd and the quartic symbol ([`weilert-hensel.md`](weilert-hensel.md))

### 3.1 Proven

1. Step H, the Hensel-quotient step, preserves the gcd (Lemma 1) and terminates with linear step bounds: 133 steps for 31-bit inputs at the offset of the earlier study, falling to 73 (Theorem 2).
2. Step S, the valuation-driven step, has linear bounds for $\delta \ge 2$ (Section 4.2).
3. Every correction to the quartic symbol depends only on residues mod 16, so the symbol can follow either step (Lemma 3, Section 5).

### 3.2 Verified by computation

1. All four symbol algorithms agree with Euler's criterion on every case tested (Section 6).
2. Step S at $\delta = 1$ terminated on all 96,800 pairs with coordinates up to 10, and on every random case (4.3).
3. The quotients depend only on low digits, as a recursive version would need, in 1,500 perturbation trials per $\delta$ (4.4).

### 3.3 Conjectured

1. Step S terminates at $\delta = 1$ (4.3).
2. A truncation lemma holds for step S (4.4).

### 3.4 Relationships found

1. $\delta = 1$ sits exactly at the Gaussian weak-triangle threshold that Wikström identified (4.3).
2. In $\mathbb{Z}[i]$, unlike $\mathbb{Z}$, all symbol corrections are 2-adic, so a bottom-up quotient sequence carries the symbol with no sign problem (Section 5).

### 3.5 Open

1. Novelty: none of Weilert's papers has been read. The six sources in 8.2 must be obtained before any claim.
2. A recursive binary quartic symbol in $O(M(n) \log n)$, which would answer Weilert's 2002 remark and is the result that would make a full paper (8.3).
3. Bounds closer to the observed maxima, which are 3 to 6 times smaller than the proven ones (8.3).
