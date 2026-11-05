# Does symmetry follow from the cycle criterion?

Working notes for `papers/gaussian-collatz-2.md` (Sections 3 to 5) and `integer-census.md`. Nothing here is in the paper. Scripts: `symtest.py`, `eistest.py`, `gstopall.c`, `zany.c`, together with the existing `gcz`, `gstop` and `rules.py`. Logs are in `logs/symtest_*` and `logs/eistest_*`.

## 1. Summary

The author's criterion: a rule is Collatz-type if every nonzero orbit reaches a trivial cycle, that is, a cycle containing a unit. There are then no nontrivial cycles and no divergent orbits. Cycles are taken literally here, not up to units, because rules that are not symmetric do not commute with the units.

1. Symmetry does not follow from the criterion, not even together with minimality. At Gaussian level 2, where no symmetric rule exists (Proposition 2 of the paper), 5 of the 8 inequivalent minimal rules satisfy the criterion. Every odd $z$ with $N(z) \le 10^7$ reaches a unit under each of them. Their multipliers have norm $5$, against $29$ for $H$. At level 4, $27$ non-symmetric minimal rules pass at $N(z) \le 10^5$, $16$ of them with norm $17$. Level 6 has many more.
2. Minimality alone forces symmetry exactly where each odd class has a unique element of least norm. In $\mathbb{Z}[i]$ that is every odd level, and in $\mathbb{Z}$ every level $k \ge 2$ (Section 3). At the even Gaussian levels the classes on the boundary of the Voronoi cell have two least elements, which gives $4$, $16$ and $256$ minimal digit sets at levels $2$, $4$ and $6$, of which $0$, $2$ and $4$ are symmetric.
3. In $\mathbb{Z}$ the criterion picks out, among minimal rules, exactly the single-cycle rules of the integer census. Minimality forces symmetry from level 2 on, so nothing new appears. At level 1 none of $\pm 3x \pm 1$ passes.
4. The number of trivial cycles is at most $|U|$, and for a symmetric rule it is $|U|/|U \cap C_1|$, where $C_1$ is the cycle through $1$ (Theorem 2). A non-symmetric level-4 rule with $3$ trivial cycles shows that the divisor condition needs symmetry.
5. Conversely, symmetry and the criterion together do not force minimality, but they constrain the digits. A digit of the form $(\varepsilon\pi^v - \alpha)z$ with $v \ge k$ and $z$ an odd non-unit forces a nontrivial cycle (Proposition 3). If $\varepsilon\pi^v - \alpha$ is a unit, every digit must be a unit. This gives a proof that no symmetric Collatz-like Gaussian rule of norm less than $29$, with any digit set, satisfies the criterion (Theorem 4). It rests on the census for the digit set $U$ at level 3. So $H$'s norm is the least possible for a symmetric rule even without minimality. Among symmetric non-minimal rules at levels 3 and 4 that were tested, one passes besides $H$ and its mirror image: $\alpha = -5-2i$ with $D = U \cup (5-2i)U$.
6. In $\mathbb{Z}$ there are rules in which every orbit with $|x| \le 10^6$ falls into the unit loop $1 \leftrightarrow -1$. Examples are $-7x \pm 1$ at level 2, many rules with negative multiplier at levels 2 to 4, and some with $m = 21, 25$ at level 4 (Section 9).
7. In the Eisenstein integers, every associate of $5 + 2\omega$ and $5 + 3\omega$ at level 2 satisfies the criterion with the minimal digit set. For $8$ of these $12$ rules the trivial cycle moves between units (Section 10).

## 2. Setting and conventions

A rule at level $k$ has an odd multiplier $\alpha$ and a digit set $D$ with one element in each odd class modulo $\pi^k$, where $\pi = 1+i$ (or $2$ in $\mathbb{Z}$). It is $T(z) = z/\pi$ for even $z$ and $T(z) = \alpha z + d(z)$ for odd $z$, where $d(z)$ is the element of $D$ in the class of $-\alpha z$. It is Collatz-like if $2^k < N(\alpha) < 2^{k+1}$. $D$ is minimal if every element of $D$ has the least norm in its class. Here no symmetry is assumed, so the paper's Section 3.3 notion is extended to arbitrary tie-breaks.

Two things change when $D$ is not closed under units.

1. Associates of $\alpha$ give different rules. For symmetric $D$, $T_{u\alpha}$ is $u T_\alpha$ on odd steps, and orbits correspond up to units. For non-symmetric $D$ there is no such relation, so all associates are run.
2. The only conjugacies that remain are $z \mapsto uz$, which carry $(\alpha, D)$ to $(\alpha, u^{-1}D)$: $u^{-1}T(uz) = \alpha z + u^{-1}d(uz)$, and $u^{-1}d(uz)$ is the element of $u^{-1}D$ in the class of $-\alpha z$. Complex conjugation is not a conjugacy, because $\overline{z/(1+i)} = i\,\bar z/(1+i)$. The even step introduces a unit, which a symmetric rule absorbs and a non-symmetric one does not. The same remark shows that a non-symmetric rule depends on the choice of generator $1+i$ of the prime. The computations confirm both points: the four unit images of a digit set always give identical results, and mirror pairs $(\alpha, D)$, $(\bar\alpha, \bar D)$ often do not (Section 5.1).

In $\mathbb{Z}$ the conjugacy $x \mapsto -x$ sends $(m, D)$ to $(m, -D)$, and $-m$ is a different rule. For symmetric $D$, $T_{-m}$ is $-T_m$ on odd steps, so $\pm m$ give the same verdict, though not the same literal cycles.

Every cycle reported below was found by `gcz` (Gaussian) or `zany`/`symtest.py` (integers) and then iterated again in exact Python arithmetic, with the digit taken from the congruence.

## 3. Minimal digits and ties

*Lemma 1.* Let $D$ be minimal. For odd $z$, $d(z)$ is an element of least norm in the class $C$ of $-\alpha z$ modulo $\pi^k$. If every odd class has a unique element of least norm, there is exactly one minimal digit set, and it is symmetric.

*Proof.* The first statement is the definition. If least elements are unique, $D$ is the set of least elements, so it is unique. For a unit $u$, the class of $-\alpha(uz)$ is $uC$, and $w \mapsto uw$ is a norm-preserving bijection $C \to uC$. So the least element of $uC$ is $u$ times the least element of $C$, that is, $d(uz) = u\,d(z)$. $\square$

*Proposition 1 (where ties occur).* The elements of least norm in a class modulo the lattice $\Lambda = \pi^k\mathbb{Z}[i]$ lie in the closed Voronoi cell of $\Lambda$ at $0$, and a class has two or more least elements only if one of them lies on the boundary of the cell.

1. $k = 2j+1$ odd: $\Lambda = (1+i)2^j\mathbb{Z}[i]$, the cell is the square $|x| + |y| \le 2^j$, and its boundary points have $x + y \equiv 2^j \pmod 2$. For $j \ge 1$ they are even, so every odd class has a unique least element. For $j = 0$ (level 1) the four units are tied in the single odd class.
2. $k = 2j$ even: $\Lambda = 2^j\mathbb{Z}[i]$ and the cell is $|x|, |y| \le 2^{j-1}$. An odd boundary point has $x = \pm 2^{j-1}$ with $y$ odd, or the same with $x$ and $y$ exchanged. For $j \ge 2$ the corners are even, and each tied class has exactly two least elements, $\pm 2^{j-1} + iy$ (or the exchanged pair). This gives $2^j$ tied classes and $2^{2^j}$ minimal digit sets. For $j = 1$ the classes of $1$ and $i$ are tied, with $\{\pm 1\}$ and $\{\pm i\}$.
3. $\mathbb{Z}$: the cell of $2^k\mathbb{Z}$ is $[-2^{k-1}, 2^{k-1}]$, whose endpoints are odd only for $k = 1$. The only tie is $\pm 1$ at level 1, which is why $3x+1$ and $3x-1$ are both minimal. From level 2 on the minimal set is unique and symmetric (Lemma 2 of `integer-census.md`).

*Proof.* Every class meets the cell, and two elements of a class differ by a nonzero element of $\Lambda$. If both have least norm, they are equidistant from $0$ and from that lattice vector, so they lie on the bisector, which is the boundary. The parity statements are direct. $\square$

A symmetric minimal set is a tie-break that is constant along unit orbits. From level 3 on the units act freely on the odd classes, so the tied classes fall into $2^j/4$ orbits and there are $2^{2^{j-2}}$ symmetric minimal sets. At level 2, $-1$ fixes each class and swaps the two tied elements, so no tie-break is symmetric. That is Proposition 2 of the paper again.

Computed (`symtest.py ties`):

| level | odd classes | tied classes (norm of tied elements) | minimal digit sets | symmetric | unit orbits of minimal sets |
|---|---|---|---|---|---|
| 2 | 2 | 2 (norm 1) | 4 | 0 | 1 |
| 3 | 4 | 0 | 1 | 1 | 1 |
| 4 | 8 | 4 (norm 5) | 16 | 2 | 6 |
| 5 | 16 | 0 | 1 | 1 | 1 |
| 6 | 32 | 8 (four of norm 17, four of norm 25) | 256 | 4 | 70 |
| 7 | 64 | 0 | 1 | 1 | 1 |
| $\mathbb{Z}$, 1 | 1 | 1 ($\pm 1$) | 2 | 0 | 1 |
| $\mathbb{Z}$, 2 to 6 | $2^{k-1}$ | 0 | 1 | 1 | 1 |

The tied pairs are $\{1 \pm 2i\}$, $\{\pm 2 + i\}$ and their unit images at level 4, and $\{1 \pm 4i\}$, $\{3 \pm 4i\}$ and their unit images at level 6, as the proposition predicts. The last column counts the orbits of the minimal digit sets under $D \mapsto uD$, the number of distinct rules per multiplier. By Burnside's lemma this is $(256 + 16 + 4 + 4)/4 = 70$ at level 6 and $(16 + 4 + 2 + 2)/4 = 6$ at level 4.

So at levels 3, 5 and 7 minimality alone forces symmetry, and the criterion adds nothing to the census of the paper: the rules that pass are the symmetric single-cycle rules (none at levels 3 and 5, eight at level 7). Only the even levels can contain non-symmetric minimal rules.

## 4. The number of trivial cycles

*Theorem 2.* Let $U$ be the unit group.

1. Any rule has at most $|U|$ trivial cycles.
2. If the rule is symmetric and $1$ lies on a cycle $C_1$, the trivial cycles are the sets $uC_1$, $u \in U$. Their number is $|U|/|U \cap C_1|$, and $U \cap C_1$ is the stabilizer $\{u : uC_1 = C_1\}$, a subgroup of $U$. If $1$ lies on no cycle, no unit does, and there are no trivial cycles.

*Proof.* (1) $S$ is a function on odd elements, so distinct cycles are disjoint, and each trivial cycle contains a unit. (2) By symmetry $S(uz) = uS(z)$, so $uC$ is a cycle whenever $C$ is. A cycle through a unit $u_0$ is $u_0 C_1$, since $u_0^{-1}C$ is a cycle through $1$. So the trivial cycles form the orbit of $C_1$ under $U$, of size $|U|/|\mathrm{Stab}(C_1)|$. If $uC_1 = C_1$, then $u = u \cdot 1 \in C_1$. Conversely, if $u \in C_1$, then $uC_1$ and $C_1$ are cycles through $u$ and so are equal. Hence $\mathrm{Stab}(C_1) = U \cap C_1$. If some unit $u_0$ lies on a cycle $C$, then $1$ lies on the cycle $u_0^{-1}C$. $\square$

In $\mathbb{Z}[i]$ the count is $1$, $2$ or $4$, and in $\mathbb{Z}$ it is $1$ or $2$. The count is literal and depends on the associate of $\alpha$: replacing $\alpha$ by $u\alpha$ multiplies each odd step by $u$, and this changes which units $C_1$ contains. Checks against the computations:

| rule | trivial cycles | $U \cap C_1$ |
|---|---|---|
| $H = (5+2i, U \cup (2+i)U)$, level 4 | 2: $\{1, -1\}$ and $\{i, -i\}$ | $\{\pm 1\}$ |
| $(-5-2i)$, same digits | 4 fixed units | $\{1\}$ |
| $(2-5i)$, same digits | 1 cycle through all four units | $U$ |
| below-window $\alpha = 1+2i$, level 3, $D = U$ (paper, 3.8) | 1: $1 \to -i \to -1 \to i \to 1$ | $U$ |
| $7x \pm 1$; $-7x \pm 1$ | 2 fixed points; the loop $1 \leftrightarrow -1$ | $\{1\}$; $\{\pm 1\}$ |
| Eisenstein $5 + 2\omega$ and associates, level 2 | 1, 2, 3 or 6 | orders 6, 3, 2, 1 |
| non-symmetric level-2 rules that pass | 1 or 2 | not defined |
| non-symmetric level-4 rule $\alpha = -2-5i$, $D = U \cup \{1-2i, -2+i, 2-i, -1-2i\}$ | 3 fixed units ($1$, $-1$, $i$) | not defined |

The last row has $3$ trivial cycles, which is not a divisor of $4$. One rule in the level-6 sample also has $3$. Among the passing rules found, the counts were $1$ or $2$ at level 2; $1$, $2$, $3$ (non-symmetric) and $4$ (symmetric) at level 4; and $1$, $2$, $3$, $4$ at level 6.

## 5. Gaussian minimal rules

### 5.1 Level 2

The multipliers are the eight associates of $2 \pm i$. The four minimal sets $\{\pm 1\} \times \{\pm i\}$ form a single unit orbit, so up to conjugacy there are exactly eight rules, one per associate, with $D = \{1, i\}$. All $32$ pairs were run on $N(z) \le 10^5$ (`logs/symtest_k2_B1e5.txt`), and the four digit sets of each multiplier gave identical cycle counts and basin sizes, as Section 2 predicts. The cycles with $D = \{1, i\}$:

| $\alpha$ | trivial cycles | nontrivial cycles (odd elements) | criterion |
|---|---|---|---|
| $2 - i$ | $1 \to -i \to -1 \to 1$ | none | yes |
| $-1 - 2i$ | $1 \to -1 \to -i \to 1$ | none | yes |
| $-2 - i$ | $1 \to i \to -1 \to -i \to 1$ | none | yes |
| $1 + 2i$ | fixed points $-1$ and $-i$ | none | yes |
| $-2 + i$ | fixed points $1$ and $-i$ | none | yes |
| $2 + i$ | $1 \to -i \to -1 \to i \to 1$ | $5-2i, 1-6i, -5-4i, -6+3i, 7i$ | no |
| $1 - 2i$ | $\{i, -i\}$, $\{1, -1\}$ | $\{1+2i, -3i\}$, $\{-2-3i, 1+4i\}$ | no |
| $-1 + 2i$ | four fixed units | $\{-1-2i, -3i, 2-3i\}$, $\{2-i, 3\}$, $\{3-2i, 4-i\}$, $\{1-4i, 3-4i\}$, $\{4+i, 4+3i\}$ | no |

The pair $2 - i$ and $2 + i$ are complex conjugates and get opposite verdicts, which shows that conjugation is not a symmetry of non-symmetric rules.

For the five passing rules, `gstopall` (Section 12) checked that every odd $z$ with $1 < N(z) \le 10^7$ reaches a smaller norm, over all four quadrants, with $15{,}708{,}048$ starts per rule. There were no failures and no cycles, and the largest stopping time was $260$ steps of $T$ (`logs/symtest_k2_gstopall_1e7.txt`). By induction every orbit with $N(z) \le 10^7$ reaches a unit, and each unit lies on or leads into a trivial cycle. An independent exact iteration in Python to $N(z) \le 10^4$ agrees. In full, the rule $\alpha = 2 - i$ at level 2 is:
$$T(z) = \begin{cases} z/(1+i) & a + b \text{ even}, \\ (2-i)z + i & a \text{ odd}, b \text{ even}, \\ (2-i)z + 1 & a \text{ even}, b \text{ odd}, \end{cases}$$
with the single cycle $1 \to -i \to -1 \to 1$ in odd steps (and $S(i) = -i$). Its drift is $\ln(5/8) = -0.47$ per odd step, the largest contraction in the Collatz-like range, and the largest excursion seen was $11.4$ bits. These are the Gaussian counterpart of $3x + 1$ that Proposition 2 rules out under symmetry: a rule with one or two trivial cycles and none other found, at the first level that has Collatz-like multipliers.

### 5.2 Level 4

Level 4 has $28$ associates and $6$ classes of digit sets each, so $168$ rules, all run at $N(z) \le 10^5$ (`logs/symtest_k4_B1e5.txt`). $35$ pass:

1. $8$ symmetric: $H$ and its mirror image, each in its four associates, as in the census;
2. $27$ non-symmetric: $16$ with norm $17$ (associates $\pm 1 \pm 4i$ and $4 \pm i$) and $11$ with norm $29$.

No rule with norm $25$ passes. Under symmetry the multipliers of norm $17$ have two cycles (paper, Section 5.2), but a non-symmetric tie-break removes the second one. Two norm-17 examples were checked further with `gstopall` to $N(z) \le 10^6$, with no failures: $\alpha = 1 + 4i$ and $\alpha = 4 + i$, both with $D = U \cup \{1-2i, -2+i, -2-i, -1-2i\}$. The four associates $-4 \pm i$ and $-3 - 4i$, $-4 - 3i$ can never pass, by Corollary 3a in Section 7.

### 5.3 Level 6 (sample)

All $112 \times 70 = 7840$ rules would be too many for a light run. For each of the $112$ associates, $6$ random non-symmetric digit-set orbits were drawn, $672$ rules in all, at $N(z) \le 10^5$ with a step cap of $50000$ (`logs/symtest_k6_sample_B1e5.txt`). $146$ pass:

| norm | 65 | 73 | 81 | 85 | 89 | 97 | 101 | 109 | 113 | 117 | 121 | 125 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| passing / run | 38/96 | 34/48 | 0/24 | 19/96 | 0/48 | 0/48 | 26/48 | 23/48 | 0/48 | 6/48 | 0/24 | 0/96 |

For the real multipliers $9$ and $11$ (norms $81$, $121$) every sampled rule had escaping starts, as in the paper's Section 5.5. All $96$ rules with $\alpha = 11 \pm 2i$ (norm $125$) had unresolved starts, $90$ of them past gcz's limit of $2^{100}$. The paper needed special handling for these excursions, so the norm-125 row is inconclusive and probably reflects the cutoff. The passing norm-65 rules all have associates of $7 \pm 4i$. The associates of $8 \pm i$ cannot pass (Section 7).

## 6. Integer rules

All $m$ with $2^k < |m| < 2^{k+1}$ and every minimal digit set were run on all odd $0 < |x| \le 10^6$, with literal cycles (`logs/symtest_Z_k1_B1e6.txt`, `logs/symtest_Z_k2-5_B1e6.txt`).

| level | rules (both signs) | passing |
|---|---|---|
| 1 | 4 ($\pm 3x \pm 1$) | none: $3x+1$ has cycles at $-5$ and $-17$, $3x-1$ at $5$ and $17$, and $-3x \pm 1$ a cycle of $12$ odd steps through $\pm 13$ |
| 2 | 4 | $m = \pm 7$ |
| 3 | 8 | none |
| 4 | 16 | $m = \pm 21, \pm 25$ |
| 5 | 32 | $m = \pm 41, \pm 43, \pm 49, \pm 53$ |

These are exactly the single-cycle rules of the integer census. For $m > 0$ the trivial cycles are the fixed points $1$ and $-1$. For $m < 0$ they are the loop $1 \leftrightarrow -1$ (Theorem 2 with stabilizer $\{\pm 1\}$). In $\mathbb{Z}$ minimality forces symmetry from level 2 on, and at level 1 no minimal rule passes, so the criterion together with minimality is equivalent to the census's notion there.

## 7. The converse: what the criterion forces on symmetric digits

*Proposition 3 (forced fixed points).* Let $(\alpha, D)$ be a rule at level $k$, $v \ge k$, $\varepsilon$ a unit and $z$ odd with $\delta = (\varepsilon\pi^v - \alpha)z \in D$. Then $S(z) = \varepsilon z$. If $\varepsilon = 1$, or if the rule is symmetric, and $z$ is not a unit, the cycle through $z$ contains no unit, so the criterion fails.

*Proof.* Since $v \ge k$, $\delta \equiv -\alpha z \pmod{\pi^k}$, so $\delta$ is the digit of $z$. Then $\alpha z + \delta = \varepsilon\pi^v z$ and $S(z) = \varepsilon z$, because $z$ is odd. If $\varepsilon = 1$, $z$ is a fixed point. Under symmetry the orbit of $z$ is $\{\varepsilon^j z\}$, all of norm $N(z) > 1$. $\square$

This is Lemma 3 of `integer-census.md`, without the symmetry assumption when $\varepsilon = 1$. It forbids $D$ to meet $F_\alpha = \{(\varepsilon\pi^v - \alpha)z : v \ge k, z \text{ odd}, N(z) > 1\}$, with $\varepsilon = 1$ only for non-symmetric rules.

*Corollary 3a.* If $\varepsilon\pi^v - \alpha = \eta$ is a unit for some $v \ge k$ (with $\varepsilon = 1$ for non-symmetric rules), the criterion forces every digit to be a unit. Hence it fails for every digit set as soon as there are more odd classes than units: $k \ge 4$ in $\mathbb{Z}[i]$, $k \ge 3$ in $\mathbb{Z}$.

*Proof.* Every digit $d$ equals $(\varepsilon\pi^v - \alpha)\eta^{-1}d$, so by Proposition 3 every digit $d$ with $N(d) > 1$ gives a nontrivial fixed point $\eta^{-1}d$. $\square$

The multipliers this covers, computed for $v \le k + 2$ (larger $v$ cannot give a unit inside the window):

| level | symmetric rules (up to units) | non-symmetric rules (literal associates) |
|---|---|---|
| 2 | (no symmetric rules) | $-2+i$, $-1+2i$, $1+2i$ |
| 3 | $3$, $3 \pm 2i$ (all of level 3) | $-3$, $-3+2i$, $-2+3i$ |
| 4 | $4 \pm i$, $5$, $4 \pm 3i$ (all but $5 \pm 2i$) | $-4 \pm i$, $-5$, $-4-3i$, $-3-4i$ |
| 5 | $5 \pm 4i$, $7$ | $-5-4i$, $-4-5i$, $-7i$ |
| 6 | $8 \pm i$, $9$, $8 \pm 7i$ | $\pm 1 - 8i$, $-9i$, $7-8i$, $8-7i$ |
| 7 | $9 \pm 8i$, $15$ | $8-9i$, $9-8i$, $15$ |
| $\mathbb{Z}$, $k \ge 2$ | $m = 2^k + 1$, $2^{k+1} - 1$ | the same, for $m > 0$ |

These include the structural fixed points of the paper ($\alpha = 9$ at level 6, $15$ at level 7) and the integer rules $m = 2^k + 1$, $2^{k+1} - 1$ of Proposition 4 of `integer-census.md`.

*Theorem 4.* No symmetric Collatz-like rule in $\mathbb{Z}[i]$ with $N(\alpha) < 29$ satisfies the criterion, whatever its digit set. With minimal digits, $H$ and its mirror image are the only symmetric rules of norm $29$ that pass up to $N(z) \le 10^9$ (paper). In $\mathbb{Z}$, a symmetric rule at level 2 that satisfies the criterion has the minimal digits $\pm 1$.

*Proof.* Level 2 has no symmetric rule. At level 3 every multiplier is covered by Corollary 3a, so $D = U$, and these rules have $5$ and $4$ cycles up to units (paper, Proposition 5, cycles verified in exact arithmetic). At level 4, $4 \pm i$, $5$ and $4 \pm 3i$, of norms $17$ and $25$, are covered by Corollary 3a with $k = 4$. In $\mathbb{Z}$ at level 2, both $m = 5$ and $m = 7$ are covered, so $D = \{\pm 1\}$. $\square$

So among symmetric rules, minimality is not needed to make $H$'s norm the first. Theorem 4 relies on the level-3 census only for the digit set $U$, and on nothing else computational.

Computations (`logs/symtest_G_symnonmin_k3-4.txt`, `logs/symtest_G_symnonmin_k3_40.txt`, `logs/symtest_Z_symnonmin_*.txt`). For each unit orbit of odd classes, the digit of one class in the orbit was chosen among its $n$ smallest elements and extended symmetrically.

| ring, level | candidates per orbit | symmetric rules | passing | of which minimal |
|---|---|---|---|---|
| $\mathbb{Z}[i]$, 3 | 40 | 120 | 0 | 0 |
| $\mathbb{Z}[i]$, 4 | 5 | 175 | 3 | 2 |
| $\mathbb{Z}$, 2 | 16 ($\lvert d \rvert < 32$) | 32 | 1 ($7x \pm 1$) | 1 |
| $\mathbb{Z}$, 3 | 12 | 576 | 10 | 0 |
| $\mathbb{Z}$, 4 | 5 | 5000 | 125 | 2 |

The non-minimal Gaussian rule that passes is $\alpha = -5-2i$ (an associate of $H$'s multiplier) with $D = U \cup (5-2i)U$. Its digits of norm $29$ are far from minimal, and it has four fixed units. `gstop` confirms that every odd $z$ with $1 < N(z) \le 10^7$ drops below its norm, but with excursions of up to $80$ bits and stopping times up to $9754$ (`logs/symtest_k4_a-5-2i_D5-2i_gstop_1e7.txt`). Integer examples at level 3, where no minimal rule passes: $m = 11$ with digits $\pm 1, \pm 13$ (for the classes $\pm 1$, $\pm 3$ modulo $8$), $m = 11$ with $\pm 17, \pm 3$, $m = 13$ with $\pm 1, \pm 5$, and $m = 11$ with $\pm 47, \pm 37$.

No passing rule has a digit in $F_\alpha$, which is a consistency check of Proposition 3. Proposition 3 accounts for $3728$ of the $4981$ failures among the integer rules at levels 2 to 4, $464$ of $566$ at level 3 with $12$ candidates, and $147$ of $187$ among the Gaussian rules at levels 3 and 4. The remaining failures come from longer cycles. Apart from avoiding $F_\alpha$, the passing digit sets share no visible feature. Their digits are not bounded by the level (digits up to $47$ at integer level 3), they are not inside the Voronoi cell, and they need not be minimal in any class. So the criterion does not force minimality, even with symmetry. What it does force is Proposition 3, together with the conditions coming from longer cycles.

## 8. Non-minimal, non-symmetric rules

As a contrast, random digit sets that are neither minimal nor symmetric were tried.

1. $\mathbb{Z}[i]$, level 4: $80$ random non-symmetric digit sets with digits of norm at most $25$ in their classes, and random associates $\alpha$, at $N(z) \le 10^5$. $12$ pass, at norms $17$ (7 rules), $25$ (3) and $29$ (2) (`logs/symtest_k4_nonmin_sample.txt`). Norm 25, where no minimal rule passes, now has passing rules: $\alpha = -3+4i$, $3+4i$, $4-3i$.
2. $\mathbb{Z}$, level 2: $80$ random non-symmetric digit sets with $|d| \le 15$ and $m = \pm 5, \pm 7$, at $|x| \le 10^6$. $6$ pass, all with $m < 0$, for example $m = -5$ with digits $13$ and $3$ for the classes $1$ and $3$ modulo $4$, and $m = -7$ with $-3$ and $11$ (`logs/symtest_Z_nonmin_k2.txt`). For $m > 0$ none can pass, since Corollary 3a forces the digits $\pm 1$. For $m < 0$ the numbers $2^v - m$ are never units, so Corollary 3a gives no constraint.

So the criterion alone, without minimality, also does not force symmetry. The constraints it does impose are those of Proposition 3, which depend on $\alpha$ and the sign of $m$.

## 9. Unit loops in $\mathbb{Z}$

A loop $1 \leftrightarrow -1$ needs $S(1) = -1$ and $S(-1) = 1$. Write $d_a$ for the digit of the class of $-m$ and $d_b$ for that of $m$. Then $m + d_a = -2^v$ and $-m + d_b = 2^{v'}$ with $v, v' \ge k$. At level 1 there is one class, so $d_a = d_b$, which forces $m = -3$ and $d = \pm 1$: the rules $-3x \pm 1$. From level 2 on, $d_a = -m - 2^v$ and $d_b = m + 2^{v'}$ are forced and the other digits are free. Minimal digits and $m > 0$ give $S(1) > 0$, so a loop needs $m < 0$ or a non-minimal digit. The search (`symtest.py zloop`, `logs/symtest_Z_loop_*.txt`) used $v, v' \le k + 4$ (level 4: $k + 3$), $|d_a|, |d_b| \le 64$, and the other digits among the few smallest elements of their classes.

| level | rules with the loop | run | criterion holds | every orbit falls into the loop |
|---|---|---|---|---|
| 1 | 2 ($-3x \pm 1$) | 2 | 0 | 0 |
| 2 | 82 | 82 | 5 | 5 |
| 3 | 2500 (2 free classes, 5 options) | 2500 | 106 | 106 |
| 4 | 75816 (6 free classes, 3 options) | 3000 (sample) | 201 | 201 |

Since $1$ and $-1$ lie on the same cycle, the criterion and "every orbit falls into the loop" coincide. At level 2 the passing rules are $-7x \pm 1$ (the minimal symmetric set, $-7 = -m$ for $7x \pm 1$) and four non-symmetric rules with $m = -5$: digits $(1, 11)$, $(-11, -1)$, $(-3, 59)$ and $(-59, 3)$ for the classes $1$ and $3$ modulo $4$. At level 3 they have $m = -9$ ($102$ rules), $-11$ ($3$) and $-13$ ($1$). Two of them are symmetric, and they are the $-m$ versions of the symmetric non-minimal rules of Section 7. At level 4 most have $m < 0$, but $m = 21$ and $m = 25$ also occur, for example $m = 21$ with digits $17, 3, 37, 23, -7, -37, 13, -1$ for the classes $1, 3, \ldots, 15$ modulo $16$. The rule $7x \pm 11$ suggested in the task has the loop but fails the criterion. So yes: there are integer rules, symmetric or not and of either sign, in which every orbit with $|x| \le 10^6$ falls into the loop $1 \leftrightarrow -1$.

## 10. Eisenstein integers (internal check)

This uses the conventions of `integer-census.md`, Section 7: $\pi = 2$ (inert, $q = 4$), units $\pm 1, \pm\omega, \pm\omega^2$, and the window $4^k < N(\alpha) < 4^{k+1/3}$. Level 1 has no multiplier, since the window $(4, 6.35)$ contains no norm. Literal cycles were taken over all odd $z$ with $N(z) \le 10^5$ and all six associates of each multiplier (`eistest.py`).

1. Level 2, minimal digits. Here the minimal set is unique and symmetric, $U \cup (1-\omega)U$, so this is the symmetric census of `integer-census.md` read literally. $12$ of the $30$ rules pass: all six associates of $5 + 2\omega$ and all six of $5 + 3\omega$, norm $19$. For $8$ of these $12$ the trivial cycles move between units. One cycle runs through all six units for $-2-5\omega$, $2-3\omega$, $3+5\omega$ and $5+3\omega$. Two cycles of three units each occur for $-5-3\omega$, $-3-5\omega$, $-2+3\omega$ and $2+5\omega$, and three cycles of two units ($\pm 1$, $\pm\omega$, $\pm\omega^2$) for $-5-2\omega$ and $-3+2\omega$. The remaining two associates, $3-2\omega$ and $5+2\omega$, have six fixed units. The counts $1, 2, 3, 6$ are those of Theorem 2 (`logs/eistest_k2_min.txt`).
2. Level 3, minimal digits. One orbit of classes has tied least elements (norm $19$), so there are $2^6 = 64$ minimal sets, of which two are symmetric. A sample of $2$ random tie-breaks for each of the $96$ associates gave $192$ rules, of which $18$ pass, all with $N(\alpha) = 91$ (associates of $11 + 5\omega$ and $11 + 6\omega$). In $14$ of them the trivial cycle moves between units (`logs/eistest_k3_min_sample.txt`). The symmetric minimal level-3 rules all have at least two cycles up to units (`integer-census.md`), so this is the Eisenstein counterpart of Section 5: tie-breaking that is not symmetric produces rules that pass.
3. Level 2, non-minimal digits: of $40$ random digit sets with digits of norm at most $13$, $12$ pass (at norms $19$ and $21$), and $9$ of them have a trivial cycle through two or more units (`logs/eistest_k2_nonmin.txt`).

So in $\mathbb{Z}[\omega]$ there are rules satisfying the criterion whose trivial cycle moves between units, already among the symmetric minimal rules at level 2.

## 11. What is proven and what is computed

Proven: Lemma 1, Proposition 1 and the tie counts; Theorem 2; Proposition 3 and Corollary 3a; Theorem 4, given the paper's census of the level-3 rules with $D = U$ (cycles exhibited in exact arithmetic). Also the remarks of Section 2: associates and complex conjugates of non-symmetric rules are not conjugate in general, and $z \mapsto uz$ is a conjugacy.

Computed (the criterion is checked up to the stated bound, so these are observations):

1. Gaussian level 2: all $8$ rules up to conjugacy at $N(z) \le 10^5$. Five pass, and each sends every $z$ with $N(z) \le 10^7$ to a unit (`gstopall`).
2. Gaussian level 4: all $168$ rules at $N(z) \le 10^5$, $35$ passing, $27$ of them non-symmetric. Two norm-17 rules were also checked with `gstopall` to $10^6$.
3. Gaussian level 6: a sample of $672$ of $7840$ rules, $146$ passing. The rules with $\alpha = 11 \pm 2i$ are inconclusive because of the $2^{100}$ cutoff.
4. Integer levels 1 to 5, all minimal rules with both signs, at $|x| \le 10^6$.
5. Symmetric non-minimal rules: Gaussian levels 3 and 4 and integer levels 2 to 4, as in the table of Section 7. Integer level 5 was not run.
6. The unit-loop search and the Eisenstein check, as stated in their sections.

The answer to the question: symmetry does not fall out. Neither the cycle criterion, nor minimality, nor the two together force it. Minimality forces symmetry exactly where least-norm elements are unique, which is all odd Gaussian levels and every integer level $k \ge 2$, and this is a property of the lattice, not of the dynamics. At even Gaussian levels non-symmetric minimal rules satisfy the criterion, starting with the five level-2 rules of norm $5$. If symmetry is imposed, Theorem 4 shows that the criterion alone, with any digits, already puts the first rule at norm $29$, the norm of $H$.

## 12. Files

1. `symtest.py`: tie counts (`ties`); Gaussian minimal rules (`gauss k B`, all rules up to $D \mapsto uD$; `gsample` for the level-6 sample); Gaussian and integer symmetric non-minimal rules (`gsymnm`, `zsymnm`); random non-minimal non-symmetric samples (`gnonmin`, `inonmin`); integer minimal rules (`int`); integer unit loops (`zloop`). Gaussian runs use `gcz` through `rules.py`, and every cycle is re-iterated in exact arithmetic.
2. `zany.c` (binary `zany`): integer rule with an arbitrary digit table, literal cycles over $0 < |x| \le B$, exact 128-bit arithmetic, unresolved above $2^{100}$ or after $10^6$ steps. `gcc -O2 -o zany zany.c`.
3. `gstopall.c` (binary `gstopall`): drop-below-start check for rules that need not be symmetric, over all four quadrants, counting a cycle only on a literal return. `gcc -O2 -o gstopall gstopall.c -lm`.
4. `eistest.py`: the Eisenstein check of Section 10, built on `zeis.py`.
5. Logs: `logs/symtest_k2_B1e5.txt`, `logs/symtest_k2_gstopall_1e7.txt`, `logs/symtest_k4_B1e5.txt`, `logs/symtest_k6_sample_B1e5.txt`, `logs/symtest_Z_k1_B1e6.txt`, `logs/symtest_Z_k2-5_B1e6.txt`, `logs/symtest_G_symnonmin_k3-4.txt`, `logs/symtest_G_symnonmin_k3_40.txt`, `logs/symtest_k4_a-5-2i_D5-2i_gstop_1e7.txt`, `logs/symtest_Z_symnonmin_k2-4.txt`, `logs/symtest_Z_symnonmin_k3_12.txt`, `logs/symtest_Z_symnonmin_k2_60.txt`, `logs/symtest_k4_nonmin_sample.txt`, `logs/symtest_Z_nonmin_k2.txt`, `logs/symtest_Z_loop_k1-2.txt`, `logs/symtest_Z_loop_k3.txt`, `logs/symtest_Z_loop_k4.txt`, `logs/eistest_k2_min.txt`, `logs/eistest_k3_min_sample.txt`, `logs/eistest_k2_nonmin.txt`.
