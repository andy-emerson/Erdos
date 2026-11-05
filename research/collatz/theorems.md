# Gaussian Collatz expressions: trivial cycles, unit cycles, variations, and the norm bound 29

Working notes. These are careful proofs of results stated in `symmetry-test.md` (Theorem 2, Proposition 3, Corollary 3a, Theorem 4, the remarks of Section 2 and the level-2 rule $\alpha = 2-i$) and in `integer-census.md` (Lemma 3), with the formal Lean 4 versions in `lean/` (Section 8 lists them). Nothing here is in the paper drafts.

## 1. Setting

$\pi = 1+i$, $U = \{1, i, -1, -i\}$, $N(a+bi) = a^2+b^2$. An element $z$ is even if $\pi \mid z$ and odd otherwise; $a+bi$ is odd exactly when $a+b$ is odd. Complex conjugation is written $\bar z$. Since $\bar\pi = 1 - i = -i\pi$, conjugation preserves parity, and $\overline{\pi^k} = (-i)^k\pi^k$, so $\pi^k \mid x$ if and only if $\pi^k \mid \bar x$.

*Definition 1.1.* An expression is a triple $(\alpha, k, d)$ with $\alpha$ odd, $k \ge 0$, and a digit table $d$: a function on odd $z$ that depends only on the class of $z$ modulo $\pi^k$ (the modulus) and satisfies $\pi^k \mid \alpha z + d(z)$. The digit set is $D = \{d(z) : z \text{ odd}\}$. The map is
$$T(z) = \begin{cases} z/\pi & \pi \mid z, \\ \alpha z + d(z) & \text{otherwise}, \end{cases}$$
and the accelerated map on odd $z$ is $S(z) = w/\pi^{v(w)}$ with $w = \alpha z + d(z)$, where $v(w)$ is the exponent of $\pi$ in $w \neq 0$, so that $S(z)$ is odd and $v(w) \ge k$. If $w = 0$ we put $S(z) = 0$, and $S(0) = 0$; this can happen only when $d(z) = -\alpha z$, and then the orbit of $z$ ends at $0$.

*Lemma 1.2.* (a) $\pi \mid xy$ implies $\pi \mid x$ or $\pi \mid y$. (b) If $\alpha$ is odd and $\pi^k \mid \alpha x$, then $\pi^k \mid x$. (c) If $k \ge 1$, every digit is odd, and $D$ has exactly one element in each odd class modulo $\pi^k$: if $d(z) \equiv d(z') \pmod{\pi^k}$ then $d(z) = d(z')$.

*Proof.* (a) Write $x = x_1 + x_2 i$, $y = y_1 + y_2 i$. The real and imaginary parts of $xy$ sum to $x_1y_1 - x_2y_2 + x_1y_2 + x_2y_1 \equiv (x_1+x_2)(y_1+y_2) \pmod 2$, so $xy$ is odd when $x$ and $y$ are. (b) By induction on $k$. For $k \ge 1$, (a) gives $\pi \mid x$, say $x = \pi x'$. Then $\pi^k \mid \pi\alpha x'$, and cancelling $\pi$ (a nonzero element of a domain) gives $\pi^{k-1} \mid \alpha x'$, so $\pi^{k-1} \mid x'$ and $\pi^k \mid x$. (c) $d(z) \equiv -\alpha z \pmod \pi$, which is odd by (a). If $d(z) \equiv d(z')$, then the difference $\alpha(z' - z) + (d(z') - d(z))$ of the multiples $\alpha z' + d(z')$ and $\alpha z + d(z)$ of $\pi^k$ is a multiple of $\pi^k$, and so is $d(z') - d(z)$, hence $\pi^k \mid \alpha(z' - z)$. By (b), $z \equiv z'$, so $d(z) = d(z')$ because the table depends only on the class. $\square$

So, for $k \ge 1$, $d(z)$ is the element of $D$ in the class of $-\alpha z$, as in the paper.

*Definition 1.3.* For a map $f$ on $\mathbb{Z}[i]$, a point $x$ is periodic if $f^p(x) = x$ for some $p \ge 1$, and its cycle is $\{f^j(x) : j \ge 0\}$. A cycle is trivial if it contains a unit, and a unit cycle if all its elements are units. The map is trivial if every odd $z$ has an iterate $f^n(z)$ that is periodic with a trivial cycle. An expression is a rule if its map $S$ is trivial. The expression is symmetric if $d(uz) = u\,d(z)$ for every unit $u$ and every odd $z$.

*Lemma 1.4.* (a) If $x$ is periodic for $f$ and $y = f^j(x)$, then $x = f^m(y)$ for some $m$. Hence two cycles that meet are equal, and the cycle of $y$ is the cycle of $x$. (b) The expression is symmetric if and only if $T(uz) = uT(z)$ for all units $u$ and all $z$, if and only if $D$ is closed under multiplication by units (for $k \ge 1$). (c) If it is symmetric, $S(uz) = uS(z)$ and $S^n(uz) = uS^n(z)$ for all units $u$, all $z$ and all $n$.

*Proof.* (a) If $f^p(x) = x$ with $p \ge 1$, take $m = j(p-1)$: $f^m(y) = f^{jp}(x) = x$. (b) For even $z$, $T(uz) = uz/\pi = uT(z)$ always, and $uz$ is even. For odd $z$, $T(uz) = uT(z)$ says $\alpha uz + d(uz) = u\alpha z + u\,d(z)$. If $D$ is closed under units, $u\,d(z) \in D$ lies in the class of $-\alpha uz$, so it is $d(uz)$ by Lemma 1.2(c); the converse is clear. (c) For odd $z$, $\alpha uz + d(uz) = u(\alpha z + d(z))$, and multiplication by a unit does not change the exponent of $\pi$, so $S(uz) = uS(z)$; both sides are $0$ for even $z$ and when $w = 0$. Induction gives the iterates. $\square$

*Remark 1.5.* Triviality can be read off either $T$ or $S$. A nonzero even $z$ reaches its odd part under $T$. From odd $z$ the $T$-orbit runs through $S(z)$, $S^2(z)$, \ldots, with only even elements between consecutive terms, and units are odd. So a nonzero $T$-orbit reaches a $T$-cycle containing a unit exactly when the $S$-orbit of the first odd element reaches an $S$-cycle containing a unit. The proofs below use $S$.

## 2. The number of trivial cycles

*Theorem 2.1.* (a) For any map $f$ on $\mathbb{Z}[i]$, in particular for $S$ of any expression, there are at most $|U| = 4$ trivial cycles.

(b) Let $f$ satisfy $f(uz) = uf(z)$ for all units $u$ and all $z$ (for instance $S$ of a symmetric expression). If $1$ is not periodic, no unit is periodic and there are no trivial cycles. If $1$ is periodic, with cycle $C_1$, then:

1. for each unit $u$, $u$ is periodic and its cycle is $uC_1$;
2. every trivial cycle is $uC_1$ for some unit $u$;
3. $uC_1 = C_1$ if and only if $u \in C_1$, so $H = U \cap C_1$ is the stabilizer of $C_1$ in $U$, and it is a subgroup;
4. the trivial cycles are the cosets $uC_1$, and their number is $|U|/|H|$, a divisor of $4$. Exactly one of the following holds: $H = \{1\}$, and $C_1, iC_1, -C_1, -iC_1$ are four distinct trivial cycles; $H = \{\pm 1\}$, and there are two, $C_1 = -C_1$ and $iC_1 = -iC_1 \neq C_1$; $H = U$, and there is one.

*Proof.* (a) Each trivial cycle contains a unit; choose one. By Lemma 1.4(a) distinct cycles are disjoint, so distinct trivial cycles give distinct units.

(b) By Lemma 1.4(c), $f^n(u) = uf^n(1)$, so $f^p(u) = u$ if and only if $f^p(1) = 1$, and $\{f^j(u)\} = u\{f^j(1)\}$. This gives the first statement and item 1. For item 2, a trivial cycle contains a unit $u$, so by Lemma 1.4(a) it is the cycle of $u$, which is $uC_1$. For item 3, if $uC_1 = C_1$ then $u = u \cdot 1 \in C_1$; conversely, if $u \in C_1$, then $uC_1$ and $C_1$ are cycles through $u$, so they are equal. $H$ contains $1$, and it is closed under products: if $u, u' \in C_1$, then $uu' \in uC_1 = C_1$. A subset of the cyclic group $U$ of order $4$ that contains $1$ and is closed under products is a subgroup, and it is $\{1\}$, $\{\pm 1\}$ or $U$, because $i^2 = (-i)^2 = -1$ and $i \cdot (-1) = -i$. For item 4, $uC_1 = u'C_1$ if and only if $u^{-1}u' \in H$, so the trivial cycles correspond to the cosets of $H$ in $U$, and the three cases are read off. $\square$

The count is literal (cycles as sets, not up to units). Section 4 of `symmetry-test.md` lists a non-symmetric modulus-$\pi^4$ rule with $3$ trivial cycles, so (b) needs symmetry; (a) does not.

## 3. Unit cycles

*Theorem 3.1.* Let $f$ satisfy $f(uz) = uf(z)$ for all units $u$ and all $z$, for instance $S$ of a symmetric expression, and put $w = f(1)$.

1. If $w$ is a unit, then $f(u) = uw$ for every unit $u$, so $f$ permutes the units by multiplication by $w$, and $f^n(u) = uw^n$. Every unit is periodic, its cycle consists of units, and $f^n(u) = u$ if and only if $w^n = 1$. So every unit cycle has length equal to the order of $w$ ($1$ for $w = 1$, $2$ for $w = -1$, $4$ for $w = \pm i$), the units form $4/\mathrm{ord}(w)$ unit cycles, and no unit lies off a unit cycle.
2. If $w$ is not a unit, then $f(u) = uw$ is not a unit for any unit $u$, so there is no unit cycle.

*Proof.* $f(u) = f(u \cdot 1) = uf(1) = uw$, and by induction $f^n(u) = uw^n$. If $w$ is a unit, so is $uw^n$, and $uw^n = u$ if and only if $w^n = 1$. The order of a unit is as stated, since $(-1)^2 = 1$ and $i^2 = (-i)^2 = -1$. If $w$ is not a unit, $N(uw) = N(w) \neq 1$. $\square$

In particular, under symmetry the units are on one footing: a unit cycle exists exactly when $S(1)$ is a unit, and then every unit lies on one.

*Remark 3.2 (without symmetry).* Consider the expression $E_2$ with $\alpha = 2 - i$ at modulus $\pi^2 = 2i$, with digit $d(a+bi) = i$ for $a$ odd, $b$ even, and $d(a+bi) = 1$ for $a$ even, $b$ odd (Section 7 checks it is an expression). It is not symmetric, as no expression at modulus $\pi^2$ is (Section 5): $d(-1) = i \neq -d(1)$. Here
$$S(1) = \frac{2}{\pi^2} = -i, \quad S(-i) = \frac{-2i}{\pi^2} = -1, \quad S(-1) = \frac{-2+2i}{\pi^2} = 1, \quad S(i) = \frac{2+2i}{\pi^3} = -i,$$
so $1 \to -i \to -1 \to 1$ is a unit cycle of length $3$, which is not the order of any unit, and the unit $i$ lies off it and maps into it. Both conclusions of Theorem 3.1(1) fail, although $S(1)$ is a unit.

## 4. Rotation and reflection

*Definition 4.1.* For a unit $u$, the rotated expression of $(\alpha, k, d)$ is $(u\alpha, k, d^u)$ with $d^u(z) = d(uz)$. It is an expression with the same digit set: $d^u$ depends only on the class of $z$, because $z \equiv z'$ implies $uz \equiv uz'$, and $u\alpha z + d(uz) = \alpha(uz) + d(uz)$ is a multiple of $\pi^k$ because $uz$ is odd. So $d^u(z)$ is the element of $D$ in the class of $-u\alpha z$. When $d$ is symmetric, $d^u(z) = u\,d(z)$.

*Proposition 4.2 (no symmetry needed).* $S_{u\alpha}(z) = S_\alpha(uz)$ for all $z$.

*Proof.* For odd $z$, $u\alpha z + d^u(z) = \alpha(uz) + d(uz)$, and $uz$ is odd. For even $z$ both sides are $0$. $\square$

*Lemma 4.3 (transfer).* Let $f$ and $g$ satisfy $f(uz) = uf(z)$ and $g(uz) = ug(z)$ for all units $u$ and all $z$, and suppose that for every $z$ there is a unit $\varepsilon$ with $g(z) = \varepsilon f(z)$. If $f$ is trivial, so is $g$.

*Proof.* By induction, $g^n(z) = \delta_n f^n(z)$ with $\delta_n$ a unit: $g^{n+1}(z) = g(\delta_n f^n(z)) = \delta_n g(f^n(z)) = \delta_n \varepsilon f^{n+1}(z)$. Let $z$ be odd, and let $y = f^n(z)$ be periodic, $f^p(y) = y$ with $p \ge 1$, with $f^j(y)$ a unit. Put $x = g^n(z) = \delta y$. Then $g^p(x) = \delta g^p(y) = \delta\delta' f^p(y) = \delta' x$ for a unit $\delta'$, and since $g^p$ commutes with units, $g^{mp}(x) = \delta'^m x$; with $m = 4$, $g^{4p}(x) = x$. So $x$ is periodic for $g$, and $g^j(x) = \delta g^j(y) = \delta\delta'' f^j(y)$ is a unit. $\square$

*Theorem 4.4 (rotation).* Let the expression $(\alpha, k, d)$ be symmetric and $u$ a unit. Then:

1. the rotated expression is symmetric, and $S_{u\alpha}(z) = uS_\alpha(z)$ for all $z$;
2. $S_{u\alpha}^n(z) = u^n S_\alpha^n(z)$, so $N(S_{u\alpha}^n(z)) = N(S_\alpha^n(z))$ for all $n$;
3. the rotated expression is a rule if and only if the original is;
4. $S_{u\alpha}(1) = uw$ with $w = S_\alpha(1)$, so its trivial cycles are described by Theorem 3.1 with $uw$ in place of $w$ (and by Theorem 2.1).

*Proof.* (1) $d^u(vz) = d(uvz) = uv\,d(z) = v\,d^u(z)$, and by Proposition 4.2 and Lemma 1.4(c), $S_{u\alpha}(z) = S_\alpha(uz) = uS_\alpha(z)$. (2) By induction, using that $S_\alpha$ commutes with units: $S_{u\alpha}^{n+1}(z) = uS_\alpha(u^nS_\alpha^n(z)) = u^{n+1}S_\alpha^{n+1}(z)$. Units have norm $1$. (3) Lemma 4.3 in both directions, with $\varepsilon = u$ and $\varepsilon = u^{-1}$. (4) is (1) at $z = 1$. $\square$

Only Proposition 4.2 holds without symmetry. Without it $S_{u\alpha}$ is $S_\alpha$ precomposed with multiplication by $u$, and the iterates are not related.

*Definition 4.5.* The mirrored expression of $(\alpha, k, d)$ is $(\bar\alpha, k, \bar d)$ with $\bar d(z) = \overline{d(\bar z)}$. It is an expression with digit set $\bar D$: if $\pi^k \mid z - z'$ then $\pi^k \mid \bar z - \bar z'$, so $\bar d$ depends only on the class; and $\bar\alpha z + \bar d(z) = \overline{\alpha\bar z + d(\bar z)}$ is a multiple of $\pi^k$ because $\bar z$ is odd.

*Proposition 4.6 (no symmetry needed).* For odd $z$ put $w = \alpha\bar z + d(\bar z)$. If $w \neq 0$,
$$S_{\bar\alpha}(z) = (-i)^{v(w)}\,\overline{S_\alpha(\bar z)},$$
and in all cases $N(S_{\bar\alpha}(z)) = N(S_\alpha(\bar z))$.

*Proof.* Write $w = \pi^v y$ with $v = v(w)$ and $y = S_\alpha(\bar z)$ odd. Then $\bar\alpha z + \bar d(z) = \bar w = \bar\pi^v\bar y = \pi^v\,(-i)^v\bar y$, and $(-i)^v\bar y$ is odd, so it is $S_{\bar\alpha}(z)$. If $w = 0$ both sides are $0$, and for even $z$ both maps give $0$. $\square$

So one step of the mirrored map is the conjugate of one step of the original from $\bar z$, times the unit $(-i)^v$ coming from $\overline{z/\pi} = i\,\bar z/\pi$ at each of the $v$ divisions.

*Theorem 4.7 (reflection).* Let the expression be symmetric. Then the mirrored expression is symmetric, and for all $z$ and $n$
$$S_{\bar\alpha}^n(z) = (-i)^{V_n(\bar z)}\,\overline{S_\alpha^n(\bar z)}, \qquad V_n(y) = \sum_{j=0}^{n-1} v\big(\alpha S_\alpha^j(y) + d(S_\alpha^j(y))\big),$$
where a term is $0$ when $S_\alpha^j(y)$ is even or the sum $\alpha S^j_\alpha(y) + d(S^j_\alpha(y))$ is $0$. In particular $N(S_{\bar\alpha}^n(z)) = N(S_\alpha^n(\bar z))$ for all $n$: the norm sequence of $z$ under the mirrored expression is the norm sequence of $\bar z$ under the original. The mirrored expression is a rule if and only if the original is.

*Proof.* Symmetry: $\bar d(uz) = \overline{d(\bar u\bar z)} = \overline{\bar u\,d(\bar z)} = u\,\bar d(z)$. The formula is proved by induction on $n$. For $n + 1$, by Proposition 4.6, $S_{\bar\alpha}(z) = \varepsilon\,\overline{S_\alpha(\bar z)}$ with $\varepsilon = (-i)^{v_0}$, $v_0$ the first term of $V_{n+1}(\bar z)$. By the symmetry of the mirror and the induction hypothesis applied at $\overline{S_\alpha(\bar z)}$,
$$S_{\bar\alpha}^{n+1}(z) = \varepsilon\,S_{\bar\alpha}^n\big(\overline{S_\alpha(\bar z)}\big) = \varepsilon\,(-i)^{V_n(S_\alpha(\bar z))}\,\overline{S_\alpha^{n}(S_\alpha(\bar z))} = (-i)^{V_{n+1}(\bar z)}\,\overline{S_\alpha^{n+1}(\bar z)}.$$
For the last statement, let $h(z) = \overline{S_\alpha(\bar z)}$. It commutes with units, $h(uz) = \overline{\bar u S_\alpha(\bar z)} = uh(z)$, and $h^n(z) = \overline{S_\alpha^n(\bar z)}$, so $h$ is trivial exactly when $S_\alpha$ is (conjugation preserves parity, periodicity and units). By Proposition 4.6, $S_{\bar\alpha}(z) = \varepsilon(z) h(z)$ with a unit $\varepsilon(z)$, so Lemma 4.3 applies in both directions. $\square$

Without symmetry only Proposition 4.6 remains. The second step of the mirrored orbit is applied to $(-i)^v\overline{S_\alpha(\bar z)}$, not to $\overline{S_\alpha(\bar z)}$, and a non-symmetric table treats these differently; this is the remark of `symmetry-test.md`, Section 2, and the pair $2 \pm i$ at modulus $\pi^2$ gets opposite verdicts there.

The eight symmetric variations of $H = (5+2i, U \cup (2+i)U)$ at modulus $\pi^4$ (`variants29.py`, `mirror29.py`) are the four rotations of $H$ and the four rotations of its mirror $(5-2i, U \cup (2-i)U)$. By Theorems 4.4 and 4.7, the norm sequence of $z$ is that of $z$ under $H$ for the rotations and that of $\bar z$ under $H$ for the rotated mirrors, which is what `mirror29.py` checks numerically, and all eight are rules if and only if $H$ is.

## 5. No symmetric rule inside the window below norm 29

*Lemma 5.1 (fixed points).* Let $(\alpha, k, d)$ be an expression, $v \ge k$, $\varepsilon$ a unit, $z$ odd, and suppose $\delta = (\varepsilon\pi^v - \alpha)z$ lies in $D$, say $\delta = d(z_0)$. Then $d(z) = \delta$ and $S(z) = \varepsilon z$. If moreover $z$ is not a unit, and the expression is symmetric or $\varepsilon = 1$, then the cycle of $z$ is contained in $\{z, iz, -z, -iz\}$, contains no unit, and the expression is not a rule.

*Proof.* $\alpha z_0 + \delta$ and $\varepsilon\pi^v z = \alpha z + \delta$ are multiples of $\pi^k$, so $\pi^k \mid \alpha(z_0 - z)$, hence $z_0 \equiv z$ by Lemma 1.2(b) and $d(z) = d(z_0) = \delta$. Then $\alpha z + d(z) = \pi^v(\varepsilon z)$ with $\varepsilon z$ odd, so $S(z) = \varepsilon z$. Under symmetry (or trivially if $\varepsilon = 1$), $S^n(z) = \varepsilon^n z$, so $z$ is periodic and every element of its orbit has norm $N(z) \neq 1$. No iterate of $z$ lies on a trivial cycle: every iterate of $z$ is of the form $\varepsilon^n z$, and its cycle is that of $z$. $\square$

This is Lemma 3 of `integer-census.md` and Proposition 3 of `symmetry-test.md`.

*Corollary 5.2.* Let $k \ge 1$, $v \ge k$ and $\varepsilon$ a unit with $\eta = \varepsilon\pi^v - \alpha$ a unit. If the expression is a rule, and it is symmetric or $\varepsilon = 1$, then every digit is a unit.

*Proof.* Let $\delta = d(z_0)$, odd by Lemma 1.2(c). Put $z = \eta^{-1}\delta$, which is odd, with $(\varepsilon\pi^v - \alpha)z = \delta$. By Lemma 5.1, if $\delta$ were not a unit, then $z$ would not be one either, and the expression would not be a rule. $\square$

*Lemma 5.3.* Let $\alpha$ be odd and $2^k < N(\alpha) < 2^{k+1}$ with $N(\alpha) < 29$. Then $k \in \{2, 3, 4\}$, and $\alpha = u\alpha_0$ for a unit $u$ and

1. $k = 2$: $N(\alpha) = 5$;
2. $k = 3$: $\alpha_0 \in \{3, 3+2i, 3-2i\}$ ($N = 9, 13, 13$);
3. $k = 4$: $\alpha_0 \in \{4+i, 4-i, 5, 4+3i, 4-3i\}$ ($N = 17, 17, 25, 25, 25$).

*Proof.* $N(\alpha) = a^2 + b^2$ with $a + b$ odd, so $N(\alpha)$ is odd, and $N(\alpha) > 2^k$ with $N(\alpha) < 29 < 32$ gives $k \le 4$. For $k \le 1$ the window $(2^k, 2^{k+1})$ contains no odd sum of two squares ($3$ is not one). For $k = 2, 3, 4$ the odd sums of two squares in the window are $5$; $9, 13$; $17, 25$ (not $19, 21, 23, 27$), and the elements of these norms are the listed associates, for instance $2 + 3i = i(3 - 2i)$, $-1 + 4i = i(4 + i)$, $3 + 4i = i(4 - 3i)$. $\square$

*Theorem 5.4.* Let $(\alpha, k, d)$ be a symmetric expression with $2^k < N(\alpha) < 2^{k+1}$ (inside the window) and $N(\alpha) < 29$. Then it is not a rule, whatever the digit table.

*Proof.* By Lemma 5.3, $k \in \{2, 3, 4\}$.

$k = 2$. There is no symmetric expression with $1 \le k \le 2$: since $-1 - 1 = -2 = i\pi^2$, $-1 \equiv 1 \pmod{\pi^k}$, so $d(-1) = d(1)$, while symmetry gives $d(-1) = -d(1)$. Hence $d(1) = 0$, which is even, against Lemma 1.2(c).

$k = 3, 4$. Write $\alpha = u\alpha_0$ as in Lemma 5.3. The rotation of the expression by $u^{-1}$ has multiplier $\alpha_0$, is symmetric, and is a rule if and only if the original is (Theorem 4.4). So we may assume $\alpha = \alpha_0$. In each case there is a unit $\varepsilon$ and $v \ge k$ with $\eta = \varepsilon\pi^v - \alpha$ a unit, using $\pi^3 = -2+2i$, $\pi^4 = -4$, $\pi^5 = -4-4i$:

| $k$ | $\alpha$ | $\varepsilon$ | $v$ | $\varepsilon\pi^v$ | $\eta$ |
|---|---|---|---|---|---|
| 3 | $3$ | $-1$ | 4 | $4$ | $1$ |
| 3 | $3+2i$ | $-i$ | 3 | $2+2i$ | $-1$ |
| 3 | $3-2i$ | $-1$ | 3 | $2-2i$ | $-1$ |
| 4 | $4+i$ | $-1$ | 4 | $4$ | $-i$ |
| 4 | $4-i$ | $-1$ | 4 | $4$ | $i$ |
| 4 | $5$ | $-1$ | 4 | $4$ | $-1$ |
| 4 | $4+3i$ | $-1$ | 5 | $4+4i$ | $i$ |
| 4 | $4-3i$ | $i$ | 5 | $4-4i$ | $-i$ |

Suppose the expression is a rule. By Corollary 5.2 every digit is a unit.

For $k = 4$ this is impossible. Each $\alpha$ in the table is congruent to a unit $u'$ modulo $4 = -\pi^4$ ($4 \pm i \equiv \pm i$, $5 \equiv 1$, $4 \pm 3i \equiv \mp i$). Take $z_0 = 1 + 2i$. If $d(z_0) = e$ were a unit, then $4 \mid u'(1+2i) + e$, that is, $1 + 2i \equiv -u'^{-1}e \pmod 4$; but $1+2i$ minus a unit is $2i$, $1+i$, $2+2i$ or $1+3i$, none divisible by $4$.

For $k = 3$, two distinct units differ by an element of norm $2$ or $4$, while a nonzero multiple of $\pi^3$ has norm at least $8$. So the units lie in distinct classes modulo $\pi^3$, and since all digits are units, $d(z)$ is the unique unit $e$ with $\pi^3 \mid \alpha z + e$; in other words $D = U$, the minimal digit set. These expressions have nontrivial cycles. The following steps, written $z \xrightarrow{e,\,v} S(z)$ with $\alpha z + e = \pi^v S(z)$, are checked by direct multiplication:

1. $\alpha = 3$: $-2-i \xrightarrow{i,\,3} 1+2i \xrightarrow{-1,\,3} 1-2i = i(-2-i)$;
2. $\alpha = 3+2i$: $-3 \xrightarrow{-1,\,3} 1+4i \xrightarrow{-1,\,3} 5-2i \xrightarrow{1,\,5} -3+2i \xrightarrow{1,\,4} 3 = -(-3)$;
3. $\alpha = 3-2i$: $-3 \xrightarrow{-1,\,3} 4+i \xrightarrow{-i,\,3} -5-2i \xrightarrow{-1,\,5} 2-3i \xrightarrow{i,\,4} 3i = -i(-3)$.

By symmetry, the set $P$ of unit multiples of the listed points is mapped into itself by $S$: $S(cz) = cS(z)$, and each listed $S(z)$ is a unit multiple of a listed point. No element of $P$ is a unit (the norms are $5$; $9, 17, 29, 13$; $9, 17, 29, 13$). So the orbit of the odd element $z_0 = -2-i$, respectively $-3$, stays in $P$, and no iterate lies on a trivial cycle. The literal cycles have $8$, $8$ and $16$ elements. $\square$

The window is part of the hypothesis and cannot be dropped. Below it, for $\alpha = 1 + 2i$ ($N = 5 < 8$) at modulus $\pi^3$, Corollary 5.2 also forces $D = U$ ($\varepsilon = -i$, $v = 3$, $\eta = 1$), and this symmetric expression is a rule: every odd step from a non-unit lowers the norm, and $S(1) = -i$, so the units form one unit cycle (paper, Section 3.8, and Theorem 3.1 with $w = -i$). So at $k = 3$ the proof rests on the explicit cycles, which exist inside the window. Theorem 5.4 also uses no minimality: the digit table is arbitrary, and the conclusion is that among symmetric expressions inside the window, $29$, the norm of $H$, is the least norm a rule can have. That $H$ itself is a rule is checked by computer to $N(z) \le 10^9$ (paper), not proved.

## 6. Soundness of the drop-below-start check without symmetry

*Theorem 6.1.* Let $T$ be any map on $\mathbb{Z}[i]$ with $T(z) = z/\pi$ for even $z$, and let $N_0 \le B$. Suppose that every odd $z$ (in every quadrant) with $N_0 < N(z) \le B$ has some $n$ with $N(T^n(z)) < N(z)$. Then every $z$ with $N(z) \le B$ has some $n$ with $N(T^n(z)) \le N_0$.

*Proof.* First, every $z$ with $N_0 < N(z) \le B$ has an iterate of smaller norm: for odd $z$ by hypothesis, and for even $z$ with $n = 1$, since $N(z/\pi) = N(z)/2 < N(z)$ ($z \neq 0$ because $N(z) > N_0 \ge 0$). Then by strong induction on $N(z)$: if $N(z) \le N_0$ take $n = 0$; otherwise take $n$ with $N(T^n(z)) < N(z) \le B$ and apply the induction hypothesis to $T^n(z)$, using $T^{n+n'} = T^{n'} \circ T^n$. $\square$

This is the check of `gstopall.c`, which runs every odd $z$ with $B_1 < N(z) \le B_2$ in all four quadrants and stops at the first iterate of $T$ of smaller norm. Unlike the drop-below-start check for symmetric rules (Lean `drop_below_start_sound`), it needs no relation between $T(uz)$ and $T(z)$, at the price of four times as many starts.

*Proposition 6.2.* The expression $E_2$ of Remark 3.2 ($\alpha = 2 - i$, modulus $\pi^2$) satisfies:

1. its table is a digit table: for odd $z$, $\pi^2 \mid (2-i)z + d(z)$, and $d$ depends only on the class of $z$ modulo $\pi^2$;
2. $T(z) = z/\pi$ for even $z$;
3. $S(1) = -i$, $S(-i) = -1$, $S(-1) = 1$ and $S(i) = -i$, so $1 \to -i \to -1 \to 1$ is a unit cycle and $i$ is not periodic;
4. $T(z) \neq 0$ for $z \neq 0$.

*Proof.* (1) $\pi^2 = 2i$, so classes modulo $\pi^2$ are classes modulo $2$, and the odd classes are those of $1$ ($a$ odd, $b$ even) and $i$ ($a$ even, $b$ odd). Modulo $2$, $(2-i)z + d(z) \equiv iz + d(z)$, which is $i(a + bi) + i \equiv i(a+1) - b \equiv 0$ in the first case and $i a - b + 1 \equiv 0$ in the second. (2) is the definition. (3) $(2-i) + i = 2 = \pi^2(-i)$; $(2-i)(-i) + 1 = -2i = \pi^2(-1)$; $-(2-i) + i = -2 + 2i = \pi^2 \cdot 1$; $(2-i)i + 1 = 2 + 2i = \pi^3(-i)$. The $S$-orbit of $i$ is $i, -i, -1, 1, -i, \ldots$, which never returns to $i$. (4) For even $z \neq 0$, $z/\pi \neq 0$. For odd $z$, $(2-i)z + d(z) = 0$ would give $5N(z) = N(d(z)) = 1$. $\square$

*Corollary 6.3.* Suppose the check of Theorem 6.1 holds for $E_2$ with $N_0 = 1$ and bound $B$: every odd $z$ with $1 < N(z) \le B$ has $n$ with $N(T^n(z)) < N(z)$ (`gstopall` with $B_1 = 1$, $B_2 = B$, reporting no failures and no cycles). Then every nonzero $z$ with $N(z) \le B$ has an iterate $T^n(z) \in \{1, -i, -1\}$, the unit cycle of Proposition 6.2, so every such orbit reaches the trivial cycle. The run logged in `symmetry-test.md`, Section 5.1, used $B = 10^7$.

*Proof.* By Theorem 6.1, some $T^n(z)$ has norm at most $1$, so it is $0$ or a unit. It is not $0$, by Proposition 6.2(4) and induction. If it is $i$, then $T^4(i) = -i$ (the $T$-steps are $i \to 2+2i \to 2 \to 1-i \to -i$). $\square$

## 7. What is proven

All statements above are proved in full. The only facts taken from computation are those of Corollary 6.3's hypothesis (the `gstopall` run) and the context remarks on $H$; Theorem 5.4 needs no computation beyond the finitely many multiplications displayed, which the Lean proofs check by `decide`.

## 8. Lean formalization

See `lean/README.md` for the file list. The correspondence is filled in below once the proofs build.
