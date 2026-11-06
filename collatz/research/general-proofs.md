# General proofs: results proved in the drafts and copied here

Working notes. The results below were proved only in the text of the paper drafts `collatz/papers/collatz-rings.md` (cited as the rings draft) and `collatz/papers/collatz-5p2i.md` (the 5+2i draft). Each proof is copied here, re-derived while copying, and checked by computation where a computation can check it. The checks are `genproofs.py`, with output in `logs/genproofs.txt`; the sections of that log carry the numbers of the sections below. The drafts are unchanged.

## 1. Setting

$R$ is $\mathbb{Z}$ or the ring of integers of an imaginary quadratic field, with norm $N$ ($N(x) = |x|$ on $\mathbb{Z}$, $N(z) = |z|^2$ otherwise). The norm is multiplicative, $N(z) = 1$ exactly for units, $R/(z)$ has $N(z)$ elements, and the unit group $U$ is finite. $\pi$ is a prime element dividing $2$, $q = N(\pi)$, and $z$ is odd if $\pi \nmid z$. A map $(\alpha, k, d)$ is as in Definition 1.1 of `theorems.md` with $R$ in place of $\mathbb{Z}[i]$: $\alpha$ odd, $k \ge 0$, $d$ a function on odd elements depending only on the class modulo $\pi^k$, with $\pi^k \mid \alpha z + d(z)$; $T(z) = z/\pi$ on even $z$ and $\alpha z + d(z)$ on odd $z$; $S(z) = w/\pi^{v(w)}$ with $w = \alpha z + d(z) \ne 0$, and $S(z) = 0$ if $w = 0$. A map is in the window if $q^k < N(\alpha) < q^{k + 1/(q-1)}$.

For $\mathbb{Z}[\omega]$, $z = a + b\omega$ with $\omega^2 = -1 - \omega$, $N(z) = a^2 - ab + b^2$, $\pi = 2$, $q = 4$, and $z$ is odd when $a, b$ are not both even. Conjugation sends $\omega$ to $\omega^2 = -1 - \omega$, so $\overline{a + b\omega} = (a - b) - b\omega$.

## 2. Which moduli carry multipliers

Copied from the rings draft, Proposition 6.1.

*Proposition 2.1.* The modulus $\pi^k$ carries a multiplier in the window exactly when the interval $(q^k, q^{k+1/(q-1)})$ contains the norm of an odd element. In $\mathbb{Z}$ this holds for every $k \ge 1$. In $\mathbb{Z}[i]$ and in $\mathbb{Z}[\omega]$ it holds exactly for $k \ge 2$.

*Proof.* The first statement is the definition of the window, together with the fact that every odd $\alpha$ has a map at every modulus: take for $d$ on each odd class any element of the class of $-\alpha z$, which is odd because $\alpha$ and $z$ are.

In $\mathbb{Z}$, the odd number $2^k + 1$ lies in $(2^k, 2^{k+1})$ for $k \ge 1$, and $(1, 2)$ holds no integer.

In $\mathbb{Z}[i]$, an odd $a + bi$ has $a + b$ odd, so one of $a, b$ is odd and the other even, and $a^2 + b^2 \equiv 1 \pmod 4$. The windows $(1, 2)$ ($k = 0$) and $(2, 4)$ ($k = 1$) hold no such number: the only integer in them is $3$. For $k = 2j \ge 2$, the element $2^j + i$ is odd ($2^j + 1$ is odd) and $N(2^j + i) = 4^j + 1$ lies in $(4^j, 2 \cdot 4^j) = (2^k, 2^{k+1})$. For $k = 2j + 1 \ge 3$, the element $(2^j + 1) + 2^j i$ is odd ($2^{j+1} + 1$ is odd) and its norm $2 \cdot 4^j + 2^{j+1} + 1$ lies in $(2 \cdot 4^j, 4^{j+1}) = (2^k, 2^{k+1})$, since $2^{j+1} + 1 < 2 \cdot 4^j$ for $j \ge 1$.

In $\mathbb{Z}[\omega]$, an odd $a + b\omega$ has $a, b$ not both even, and in each of the three cases ($a$ odd and $b$ even, $a$ even and $b$ odd, both odd) $a^2 - ab + b^2$ is odd. The window at $k = 0$ is $(1, 4^{1/3}) = (1, 1.587)$ and holds no integer. At $k = 1$ it is $(4, 4^{4/3}) = (4, 6.350)$, whose one odd integer is $5$, and $5$ is not a norm: $4(a^2 - ab + b^2) = (2a - b)^2 + 3b^2 = 20$ needs $3b^2 \le 20$, so $b^2 \in \{0, 1, 4\}$, and none of $20, 17, 8$ is a square. For $k \ge 2$, $z = 2^k - \omega$ is odd ($b = -1$) with $N(z) = 4^k + 2^k + 1$, which exceeds $4^k$ and lies below $4^{k+1/3}$ because $2^k + 1 < (4^{1/3} - 1)4^k = 0.5874 \cdot 4^k$ for $k \ge 2$ (at $k = 2$, $5 < 9.40$, and the right side grows faster). $\square$

*Check (log section 2).* Exhaustive over elements in a box: in $\mathbb{Z}$ for $k \le 11$ the window holds an odd integer exactly for $k \ge 1$; in $\mathbb{Z}[i]$ for $k \le 14$ the windows at $k = 0, 1$ hold no odd norm, every odd norm found in a window is $1 \bmod 4$, and the witness lies in the window for every $k \ge 2$ (norms $5, 13, 17, 41, 65, 145, \ldots$); in $\mathbb{Z}[\omega]$ for $k \le 8$ the windows at $k = 0, 1$ hold no odd norm, every norm of an odd element is odd, and $2^k - \omega$ lies in the window for $2 \le k \le 8$ (norms $21, 73, 273, \ldots$), but not at $k = 1$ (norm $7$). $5$ is not a norm in $\mathbb{Z}[\omega]$, and the inequality $2^k + 1 < (4^{1/3} - 1)4^k$ holds for $2 \le k \le 20$ and fails at $k = 1$.

## 3. Rotated and mirrored maps over a general ring

Copied from the rings draft, Definition 6.5, Proposition 6.6 and Theorem 6.7. For $\mathbb{Z}[i]$ everything here except part (3) of Theorem 3.2 is in `theorems.md` Section 4 and in Lean (`Expr.rotate_S`, `Expr.mirror_S`, `rotate_rule`, `mirror_rule`, `trivial_transfer`).

*Definitions.* For a unit $u$, the rotated map of $(\alpha, k, d)$ is $(u\alpha, k, d^u)$ with $d^u(z) = d(uz)$. Let $\sigma$ be an automorphism of $R$ of order at most $2$ with $\sigma(\pi) = \eta\pi$ for a unit $\eta$. The mirrored map is $(\sigma(\alpha), k, d^\sigma)$ with $d^\sigma(z) = \sigma(d(\sigma(z)))$. On $\mathbb{Z}[i]$, $\sigma$ is conjugation and $\eta = -i$ ($\overline{1+i} = -i(1+i)$); on $\mathbb{Z}[\omega]$, $\sigma$ is conjugation and $\eta = 1$; on $\mathbb{Z}$, $\sigma$ is the identity.

Both are maps. Since $\sigma(\pi) = \eta\pi$, $\pi \mid z$ exactly when $\pi \mid \sigma(z)$, so $\sigma$ preserves odd and even elements, and $\sigma(\pi^k R) = \pi^k R$, so $\sigma$ maps classes modulo $\pi^k$ to classes; hence $d^\sigma$ depends only on the class, and $d^u$ does because $z \mapsto uz$ maps classes to classes. Also $u\alpha z + d(uz) = \alpha(uz) + d(uz)$, a multiple of $\pi^k$, and $\sigma(\alpha)z + \sigma(d(\sigma z)) = \sigma(\alpha\sigma(z) + d(\sigma z))$, a multiple of $\sigma(\pi)^k = \eta^k\pi^k$.

*Proposition 3.1 (one step, any table).* $S_{u\alpha}(z) = S_\alpha(uz)$. For odd $z$ with $w = \alpha\sigma(z) + d(\sigma(z)) \ne 0$ and $v = v(w)$, $S_{\sigma\alpha}(z) = \eta^v\sigma(S_\alpha(\sigma z))$, so $N(S_{\sigma\alpha}(z)) = N(S_\alpha(\sigma z))$.

*Proof.* $u\alpha z + d^u(z) = \alpha(uz) + d(uz)$, and $uz$ is odd, so both sides divide the same element by the same power of $\pi$. For the mirror, $\sigma(\alpha)z + d^\sigma(z) = \sigma(w)$, and $w = \pi^v y$ with $y = S_\alpha(\sigma z)$ odd, so $\sigma(w) = \pi^v\eta^v\sigma(y)$ with $\eta^v\sigma(y)$ odd; the exponent of $\sigma(w)$ is $v$ and the odd part is $\eta^v\sigma(y)$. Automorphisms of $R$ preserve the norm (the norm is $z\sigma_0(z)$ for complex conjugation $\sigma_0$, which commutes with every automorphism of an imaginary quadratic ring, and on $\mathbb{Z}$ $\sigma$ is the identity). $\square$

*Theorem 3.2.* (1) If the table commutes with the units, so does the rotated table, $S_{u\alpha} = uS_\alpha$, $S^n_{u\alpha} = u^nS^n_\alpha$, and the rotated map is a Collatz map exactly when the original is. (2) If the table commutes with the units, so does the mirrored table, $S^n_{\sigma\alpha}(z) = \eta^{V_n}\sigma(S^n_\alpha(\sigma z))$ with $V_n$ the total exponent of the first $n$ steps from $\sigma(z)$, the norm sequence of $z$ under the mirror is that of $\sigma(z)$ under the original, and the mirror is a Collatz map exactly when the original is. (3) If $\eta = 1$, then $S_{\sigma\alpha} = \sigma \circ S_\alpha \circ \sigma$ for every table, commuting or not, and the mirror is a Collatz map exactly when the original is.

*Proof.* (1) $d^u(wz) = d(uwz) = w\,d(uz) = w\,d^u(z)$ for units $w$. Proposition 3.1 and $S_\alpha(uz) = uS_\alpha(z)$ give $S_{u\alpha} = uS_\alpha$. By induction, $S^{n+1}_{u\alpha}(z) = S_{u\alpha}(u^nS^n_\alpha(z)) = u^nS_{u\alpha}(S^n_\alpha(z)) = u^{n+1}S^{n+1}_\alpha(z)$, using that $S_{u\alpha}$ commutes with the units.

(2) $d^\sigma(uz) = \sigma(d(\sigma(u)\sigma(z))) = \sigma(\sigma(u)\,d(\sigma z)) = u\,\sigma(d(\sigma z)) = u\,d^\sigma(z)$, using $\sigma(U) = U$ and $\sigma^2 = 1$. For the iterates let $y_n = S^n_\alpha(\sigma z)$ and suppose $S^n_{\sigma\alpha}(z) = \eta^{V_n}\sigma(y_n)$. Then, since $S_{\sigma\alpha}$ commutes with the units, and Proposition 3.1 applied at the odd element $\sigma(y_n)$ (whose image under $\sigma$ is $y_n$) gives $S_{\sigma\alpha}(\sigma(y_n)) = \eta^{v_n}\sigma(S_\alpha(y_n))$, $S^{n+1}_{\sigma\alpha}(z) = \eta^{V_n}S_{\sigma\alpha}(\sigma(y_n)) = \eta^{V_n}\eta^{v_n}\sigma(S_\alpha(y_n)) = \eta^{V_{n+1}}\sigma(y_{n+1})$, with $v_n$ the exponent of the step from $y_n$. Norms follow.

(3) With $\eta = 1$, Proposition 3.1 reads $S_{\sigma\alpha}(z) = \sigma(S_\alpha(\sigma z))$ for every odd $z$ (also when $w = 0$, both sides being $0$). So $S_{\sigma\alpha} = \sigma S_\alpha \sigma$ on odd elements and $S^n_{\sigma\alpha} = \sigma S^n_\alpha \sigma$. If $S_\alpha$ is a Collatz map and $z$ is odd, then $\sigma(z)$ is odd, $S^n_\alpha(\sigma z) = y$ is periodic with a cycle $C$ containing a unit, and $S^n_{\sigma\alpha}(z) = \sigma(y)$ is periodic for $S_{\sigma\alpha}$ with cycle $\sigma(C)$, which contains the unit $\sigma$ of a unit. The mirror of the mirror is the original map ($\sigma\sigma\alpha = \alpha$, $d^{\sigma\sigma} = d$), so the converse is the same argument.

For the Collatz statements in (1) and (2): put $h = S_\alpha$ in (1) and $h = \sigma S_\alpha \sigma$ in (2); $h$ commutes with the units and is a Collatz map exactly when $S_\alpha$ is (in (2) by the argument of (3), which used only that $\sigma$ is a bijection preserving odd elements, units and cycles). The new map $g$ ($S_{u\alpha}$ or $S_{\sigma\alpha}$) commutes with the units and $g(z) = \varepsilon(z)h(z)$ with $\varepsilon(z)$ a unit. This is the transfer lemma (Lemma 4.3 of `theorems.md`, whose proof uses only that $U$ is a finite group): by induction $g^n(z) = \delta_nh^n(z)$ with $\delta_n$ a unit; if $h^n(z) = y$ lies on a trivial cycle with $h^p(y) = y$, then $x = g^n(z)$ satisfies $g^p(x) = \delta'x$, hence $g^{p|U|}(x) = x$, and the cycle of $x$ contains a unit times a unit. Exchanging $g$ and $h$ gives the converse. $\square$

Consequence for $\mathbb{Z}[\omega]$: conjugation fixes $\pi = 2$, so the mirror of every map is conjugate to it by $\sigma$, whatever the table. The mirror of $5 + 2\omega$ is $\overline{5+2\omega} = 3 - 2\omega = -\omega(5 + 3\omega)$, so with (1) the smallest-digit maps of $5 + 2\omega$ and $5 + 3\omega$ get the same verdict: the mirrored digit set $\sigma(U \cup (1-\omega)U)$ again consists of units and elements of norm $3$, which are the unique least elements of their classes modulo $4$, so it is the smallest-digit table.

*Check (log section 3).* Part (3) on $\mathbb{Z}[\omega]$ with tables that do not commute with the units: for $\alpha = 5+2\omega, 5+\omega, 5, 7+3\omega, 9+4\omega$ at moduli $4$ and $8$, five random tables each (every digit a random element of its class with coordinates up to $6$ times the modulus), the mirrored table was checked to be a digit table and $S_{\bar\alpha}(z) = \overline{S_\alpha(\bar z)}$ held at every odd $z$ with $|a|, |b| \le 30$: $138000$ points, no mismatch. Also $\overline{5+2\omega} = 3-2\omega = -\omega(5+3\omega)$.

## 4. The Markov weights are uniform

Copied from the rings draft, Lemma 2.5 and the product after it.

Write the map at modulus $\pi^k$, $k \ge 1$, as $T'$ with one branch on each class modulo $\pi^k$: $z \mapsto z/\pi$ on the $q^{k-1}$ even classes and $z \mapsto (\alpha z + d(z))/\pi^k$ on the $q^{k-1}(q-1)$ odd classes. Let $P(c, c')$, for classes $c, c'$ modulo $\pi^k$, be the fraction of the $q^k$ classes modulo $\pi^{2k}$ inside $c$ that $T'$ sends into $c'$. (This is the matrix of Matthews and Watts 1985, Section 3, read for $R$.)

*Lemma 4.1.* For every source class $c$, $\sum_{c'} P(c, c') = 1$, and for every target class $c'$, $\sum_c P(c, c') = 1$. Every class leads to every other. So the stationary distribution is unique and uniform, $q^{-k}$ on each class.

*Proof.* The value of $T'$ modulo $\pi^k$ depends only on $z$ modulo $\pi^{2k}$: if $z$ changes by $\pi^{2k}s$ within an odd class, the digit is unchanged and $(\alpha z + d)/\pi^k$ changes by $\alpha\pi^k s$; within an even class $z/\pi$ changes by $\pi^{2k-1}s$, and $2k - 1 \ge k$.

Odd source $c$: write $z = z_0 + \pi^k t$ with $t$ running over $R$ modulo $\pi^k$, which runs over the $q^k$ classes modulo $\pi^{2k}$ in $c$. Then $T'(z) = (\alpha z_0 + d)/\pi^k + \alpha t$, and since $\alpha$ is invertible modulo $\pi^k$, $t \mapsto T'(z) \bmod \pi^k$ is a bijection onto all $q^k$ classes. So $P(c, c') = q^{-k}$ for every $c'$, and the row sums to $1$.

Even source $c$: $z = \pi y$ with $y$ determined modulo $\pi^{2k-1}$ by $z$ modulo $\pi^{2k}$, and $y \equiv c/\pi \pmod{\pi^{k-1}}$. The $q^k$ classes modulo $\pi^{2k}$ in $c$ correspond to the $q^k$ classes of $y$ modulo $\pi^{2k-1}$ inside one class modulo $\pi^{k-1}$, and they fall in equal numbers $q^{k-1}$ into the $q$ classes modulo $\pi^k$ inside that class. So $P(c, c') = 1/q$ for those $q$ classes $c'$ and $0$ otherwise, and the row sums to $1$.

Target $c'$: it receives $q^{-k}$ from each of the $q^{k-1}(q-1)$ odd classes, $(q-1)/q$ in all, and $1/q$ from exactly one even class, $c = \pi\cdot(c' \bmod \pi^{k-1})$, since the even classes modulo $\pi^k$ are the classes $\pi x$ with $x$ modulo $\pi^{k-1}$. The total is $1$.

Reachability: an odd class leads to every class in one step. An even class $c = \pi^j o$ with $o$ odd and $1 \le j \le k-1$ leads in one step to classes $\equiv \pi^{j-1}o \pmod{\pi^{k-1}}$, so after $j$ steps to an odd class; the class $0$ leads to the classes $\equiv 0 \pmod{\pi^{k-1}}$, among them $\pi^{k-1}$ times a unit, and from there to an odd class in $k-1$ more steps. So within $k$ steps every class reaches an odd class, hence every class. A doubly stochastic irreducible matrix has the uniform distribution as its unique stationary distribution. $\square$

*The product.* With weights $q^{-k}$, the weighted product of the norm ratios, $1/q$ on the $q^{k-1}$ even classes and $N(\alpha)/q^k$ on the $q^{k-1}(q-1)$ odd ones, is
$$\left(\frac{1}{q}\right)^{1/q}\left(\frac{N(\alpha)}{q^k}\right)^{(q-1)/q},$$
and it is below $1$ exactly when $(q-1)\log(N(\alpha)/q^k) < \log q$, that is, $N(\alpha) < q^{k+1/(q-1)}$. Equivalently the mean change of $\log_2 N$ per step of $T'$ is $-\frac1q\log_2 q + \frac{q-1}{q}(\log_2 N(\alpha) - k\log_2 q)$, and dividing by the share $(q-1)/q$ of odd steps gives $\log_2 N(\alpha) - (k + \frac{1}{q-1})\log_2 q$ per odd step.

*Check (log section 4).* The matrix was built in exact fractions by counting the classes modulo $\pi^{2k}$ for: $\mathbb{Z}[i]$ at $(1+i)^2$ ($\alpha = 2 \pm i$), $(1+i)^3$ ($3$, $3+2i$), $(1+i)^4$ ($5+2i$, $3$, $1+4i$, $4+3i$); $\mathbb{Z}[\omega]$ at $2$ ($1+\omega$, $3$), $4$ ($5+2\omega$, $5+\omega$, $5$), $8$ ($9$, $11+5\omega$); $\mathbb{Z}$ at $2$ ($3$), $4$ ($5$, $7$), $8$ ($11$, $13$), $16$ ($21$, $29$). In every case all row and column sums are $1$, the matrix is irreducible, and the entries are $q^{-k}$ and $1/q$. The products agree with the formula (for example $0.866$ for $3x+1$, $0.935$ for $7x \pm 1$, $0.952$ for $5+2i$ and $29x+d$, $0.804$ for $5+2\omega$). The earlier check recorded in `matthews-watts1985.md` Section 3.4 lists $5 + 3i$ among its multipliers; $5 + 3i$ is even ($N = 34$) and is no multiplier, so the check here uses $4 + 3i$ in its place.

## 5. Multiplier 3 at modulus 2

Copied from the rings draft, Proposition 3.2. The fixed-point lemma on $\mathbb{Z}$ is Lemma 3 of `integer-census.md` (Proposition 3 of `symmetry-test.md` in general): for a map $(m, k, d)$ on $\mathbb{Z}$ with $k \ge 1$, $v \ge k$, $\varepsilon = \pm 1$ and odd $x$, $S(x) = \varepsilon x$ with exponent $v$ exactly when $(\varepsilon 2^v - m)x \in D$.

*Proposition 5.1.* No map with multiplier $3$ at modulus $2$ is a Collatz map on $\mathbb{Z}$.

*Proof.* There is one odd class modulo $2$, so the map has a single digit $\delta$, which is odd, and the map is $3x + \delta$. Since $(2^2 - 3)\delta = \delta \in D$ and $2 \ge k = 1$, the lemma gives $S(\delta) = \delta$ (directly: $3\delta + \delta = 4\delta$ with $\delta$ odd). If $|\delta| > 1$ this is a fixed point that is not a unit. If $\delta = 1$, the map $3x + 1$ has the cycle $-5 \to -14 \to -7 \to -20 \to -10 \to -5$ ($5$ steps of $T$, the odd elements $-5$ and $-7$) and the cycle of $18$ steps of $T$ through $-17$ (odd elements $-17, -25, -37, -55, -41, -61, -91$). If $\delta = -1$, the map $3x - 1$ is conjugate to $3x + 1$ by $x \mapsto -x$ and has the negatives of these cycles. None contains a unit. $\square$

*Check (log section 5).* $S(\delta) = \delta$ for $\delta = \pm 1, \pm 3, \pm 5, 7, 9, -11$; the accelerated cycles of $3x \pm 1$ reached from odd $|x| \le 201$ are $\{1\}$, $\{-1\}$, the pair $\{-5, -7\}$ and the $7$-element cycle through $-17$ (signs reversed for $3x - 1$); the $T$-cycles through $-5$ and $-17$ have $5$ and $18$ steps.

## 6. Least positive digits on $\mathbb{Z}$

Copied from the rings draft, Section 8 item 3, which derives the statements on $S(x) = -x$ and on $-3$ from the fixed-point lemma; the other statements are Proposition 4(3) and Section 6 of `integer-census.md`.

*Proposition 6.1.* Let $k \ge 1$, $m$ odd with $2^k < m < 2^{k+1}$, and $D = \{1, 3, \ldots, 2^k - 1\}$, the least positive residues (the digit of $x$ is $(-mx) \bmod 2^k$). Then: (1) an odd $x > 0$ is a fixed point exactly when $x < 2^k/(2^{k+1} - m)$; (2) an odd $x < 0$ is a fixed point exactly when $|x| < 2^k/(m - 2^k)$; (3) no odd $x$ has $S(x) = -x$; (4) so $1$ and $-1$ are fixed for every $m$, $3$ is fixed exactly when $m > \frac53 2^k$, and $-3$ exactly when $m < \frac43 2^k$; (5) the maps preserve $\mathbb{N}$ (and the negative integers); a map with $m > \frac53 2^k$ or $m < \frac43 2^k$ is not a Collatz map on $\mathbb{Z}$, because of the fixed point $3$ or $-3$.

*Proof.* By the lemma, $S(x) = \varepsilon x$ with exponent $v \ge k$ exactly when $\delta = (\varepsilon 2^v - m)x$ is a digit, that is, $0 < \delta < 2^k$ ($\delta$ is automatically odd). For $\varepsilon = -1$: if $x > 0$, $\delta = -(2^v + m)x < 0$; if $x < 0$, $\delta = (2^v + m)|x| > 2^k$. So (3). For $\varepsilon = 1$ and $x > 0$ we need $2^v > m$, so $v \ge k+1$; at $v = k+1$ the condition is $(2^{k+1} - m)x < 2^k$, and for $v \ge k+2$, $2^v - m > 2^{k+2} - 2^{k+1} = 2^{k+1} > 2^k$. So (1). For $x < 0$ we need $2^v < m$, so $v = k$, and the condition is $(m - 2^k)|x| < 2^k$. So (2). For (4): $2^{k+1} - m < 2^k$ and $m - 2^k < 2^k$, so $x = \pm 1$ satisfy (1) and (2); $3$ satisfies (1) iff $3(2^{k+1} - m) < 2^k$ iff $m > \frac53 2^k$; $-3$ satisfies (2) iff $3(m - 2^k) < 2^k$ iff $m < \frac43 2^k$. For (5): positive digits and $m > 0$ send a positive odd $x$ to the positive $mx + d$, and a negative odd $x$ to $mx + d \le -m + 2^k - 1 < 0$; and $\pm 3$ are fixed points that are not units. $\square$

The fixed points $1$ and $-1$ are two trivial cycles. `integer-census.md` Section 6 concludes from them that no such map has a single cycle on $\mathbb{Z}$, which is the census notion; it is no obstacle to being a Collatz map, whose trivial cycles need not be unique. In the range $\frac43 2^k < m < \frac53 2^k$ left by (4), whether these maps are Collatz maps on $\mathbb{Z}$ was not checked.

*Check (log section 6).* For $k = 1, \ldots, 8$ and every odd $m$ in the window ($255$ maps), the fixed points with $|x| < 2^{k+2}$, found by iteration, are exactly those predicted by (1) and (2); no $S(x) = -x$ in that range; $3$ is fixed iff $m > \frac53 2^k$ and $-3$ iff $m < \frac43 2^k$. No mismatch.

## 7. Score and drift of $5 + 2\omega$, and the score range

Copied from the rings draft, Sections 2.3 and 5.1.

The score is $1/M - 1/N$ with $M = N(\pi^k) = q^k$ and $N = N(\alpha)$, and the drift is $\log_2(N/M) - \log_2 q/(q-1)$. Inside the window, $q^k < N < q^{k+1/(q-1)}$, so the score lies in $(0, 1/M - q^{-k-1/(q-1)})$, which for $q = 2$ is $(0, 1/(2M))$, and the drift lies in $(-\log_2 q/(q-1), 0)$, which for $q = 2$ is $(-1, 0)$.

For $5 + 2\omega$ at modulus $4 = \pi^2$ in $\mathbb{Z}[\omega]$: $N(5 + 2\omega) = 25 - 10 + 4 = 19$, $M = N(4) = 16$, so the score is $1/16 - 1/19 = 3/304 = 0.0099$, and the drift is $\log_2(19/16) - \frac23 = 0.2479 - 0.6667 = -0.4187$ bits, $-0.2902$ in natural log, per odd step. The score range at this modulus is $(0, 1/16 - 4^{-7/3}) = (0, 0.0231)$.

*Check (log section 7).* Exact fraction $3/304$ and the figures above.

## 8. The unit-table cycles at $(1+i)^3$ for $-3$, $-2+3i$, $-3+2i$

Copied from the 5+2i draft, Theorem 2 and its proof. `modulus3-search.md` Section 4 records only that the unit table has a cycle through an element of norm $5$ or $9$.

*Theorem 8.1.* For $\alpha \in \{-3, -3+2i, -2+3i\}$ there is no Collatz map on $\mathbb{Z}[i]$ at modulus $\pi^3$, whatever the digit table.

*Proof.* With $\pi^3 = -2+2i$ and $\pi^4 = -4$: $\pi^4 - (-3) = -1$, $\pi^3 - (-3+2i) = 1$ and $\pi^3 - (-2+3i) = -i$ are units. By the forced-digit corollary (Corollary 5.2 of `theorems.md`) with $\varepsilon = 1$, which needs no condition on the units, every digit of a Collatz map is a unit. The four units lie in distinct odd classes modulo $\pi^3$ (their differences have norm $2$ or $4$, while nonzero multiples of $\pi^3$ have norm at least $8$), so the table is the unit table $D = U$. For it the following sets are invariant and contain no unit; $D = U$ commutes with the units, so the unit multiples of a cycle are cycles.

For $\alpha = -3$: $2+i \to 1+2i \to -1+2i \to -2+i \to -2-i \to -1-2i \to 1-2i \to 2-i \to 2+i$, every step with exponent $3$; these are the eight elements of norm $5$. For example $-3(2+i) + d$ with $d \equiv 3(2+i) \pmod{\pi^3}$: $d = i$ gives $-6-2i = \pi^3(1+2i)$, since $(-2+2i)(1+2i) = -6-2i$.

For $\alpha = -2+3i$: $3 \to 4-i \to 5-2i \to -2-3i \to -3 \to -4+i \to -5+2i \to 2+3i \to 3$, with exponents $3, 3, 5, 4, 3, 3, 5, 4$ and norms $9, 17, 29, 13$ twice. Its unit multiples form an invariant set of $16$ elements (this cycle and its multiple by $i$).

For $\alpha = -3+2i$: the cycle through $3$ has sixteen elements, $3 \to 4+i \to 5+2i \to 2-3i \to -3i \to \cdots$, exactly the unit multiples of $3$, $4+i$, $5+2i$ and $2-3i$ (norms $9, 17, 29, 13$), with exponents $3, 3, 5, 4$ repeated. $\square$

*Check (log section 8).* The three units $\pi^4 + 3 = -1$, $\pi^3 + 3 - 2i = 1$, $\pi^3 + 2 - 3i = -i$; iteration with the unit table, the digit of $z$ being the unique unit $e$ with $\pi^3 \mid \alpha z + e$, reproduces the three cycles with the stated exponents, none contains a unit, the unit multiples of each are invariant ($8$, $16$, $16$ elements), and the cycle for $-3+2i$ equals the set of unit multiples of $3, 4+i, 5+2i, 2-3i$. The same run reconfirms the cycles used for $\alpha = 3$, $3 \pm 2i$ in Theorem 5.4 of `theorems.md`: $-2-i \to 1+2i \to 1-2i = i(-2-i)$; $-3 \to 1+4i \to 5-2i \to -3+2i \to 3$; $-3 \to 4+i \to -5-2i \to 2-3i \to 3i$.

## 9. Digits and unit cycles over a general ring

Copied from the rings draft, Lemma 2.2 and Theorem 6.4. For $\mathbb{Z}[i]$ these are Lemma 1.2 and Theorem 3.1 of `theorems.md`, in Lean.

*Lemma 9.1.* Let $k \ge 1$. Every digit is odd, and $D$ has exactly one element in each odd class modulo $\pi^k$. So $d(z)$ is the element of $D$ in the class of $-\alpha z$, and a map is a choice of one odd element in each odd class.

*Proof.* $d(z) \equiv -\alpha z \pmod \pi$, which is odd because $\pi$ is prime and $\alpha$, $z$ are odd. If $d(z) \equiv d(z') \pmod{\pi^k}$, then $\pi^k$ divides $(\alpha z' + d(z')) - (\alpha z + d(z))$ and $d(z') - d(z)$, hence $\alpha(z' - z)$; since $\alpha$ is odd and $\pi$ is prime, $\pi^k \mid z' - z$ (cancel one factor $\pi$ at a time), so $z$ and $z'$ share a class and $d(z) = d(z')$. Every odd class $c$ is the class of $-\alpha z$ for the odd class $z \equiv -\alpha^{-1}c$, so it contains a digit. $\square$

*Theorem 9.2.* Let $f$ be a function on $R$ with $f(uz) = uf(z)$ for all units $u$ (for instance $S$ of a map that commutes with the units), and $w = f(1)$. If $w$ is a unit, then $f^n(u) = uw^n$ for every unit $u$, every unit lies on a unit cycle, each unit cycle has length $\mathrm{ord}(w)$, and there are $|U|/\mathrm{ord}(w)$ of them. If $w$ is not a unit, there is no unit cycle.

*Proof.* $f(u) = uf(1) = uw$, and by induction $f^n(u) = uw^n$, which equals $u$ exactly when $w^n = 1$; the unit cycles are the cosets $u\langle w\rangle$. If $w$ is not a unit, $N(f(u)) = N(uw) = N(w) \ne 1$, so the image of a unit is not a unit and no cycle consists of units only. $\square$

## 10. Invariants of the rings proposed as tests

Copied from the rings draft, Section 5.2, and checked here by hand. The thresholds $\kappa$ are those of `integer-census.md` Section 2.3.

1. $\mathbb{Z}[\sqrt{-2}]$, $N(a + b\sqrt{-2}) = a^2 + 2b^2$. Units $\pm 1$ ($a^2 + 2b^2 = 1$ forces $b = 0$). $2 = -(\sqrt{-2})^2$, so $\pi = \sqrt{-2}$ ramifies, $q = 2$, $e = 2$. Since $(a + b\sqrt{-2})/\sqrt{-2} = b - (a/2)\sqrt{-2}$, $a + b\sqrt{-2}$ is odd exactly when $a$ is odd. $\kappa = v_\pi(-1 - 1) = v_\pi(-2) = 2$, so commuting maps exist exactly from $\pi^3$. The window at modulus $\pi$ is $(2, 4)$, and $3 = N(1 \pm \sqrt{-2})$ with $1 \pm \sqrt{-2}$ odd, so the multipliers $\pm 1 \pm \sqrt{-2}$ lie in it (by Proposition 2.1, the modulus $\pi$ carries multipliers). Conjugation sends $\sqrt{-2}$ to $-\sqrt{-2}$, so $\eta = -1$ and the mirror carries $(-1)^v$ (Proposition 3.1).
2. $\mathbb{Z}[\theta]$, $\theta = (1 + \sqrt{-7})/2$, $\theta^2 = \theta - 2$. $\theta\bar\theta = 2$ and $\theta + \bar\theta = 1$, so $2$ splits into the two primes $\theta$, $\bar\theta$, which are not associates (the units are $\pm 1$ and $\bar\theta \ne \pm\theta$), and $\theta \nmid \bar\theta$ (else $\theta \mid 1$). So $q = 2$, $e = 1$, $\kappa = v_\theta(-2) = v_\theta(-\theta\bar\theta) = 1$: commuting maps dividing by $\theta$ exist from $\theta^2$. Conjugation sends $\theta$ to $\bar\theta$, not a unit times $\theta$, so there is no $\eta$ and no mirrored map in the sense of Section 3.
3. $\mathbb{Z}[\phi]$, $\phi = (1 + \sqrt{-11})/2$, minimal polynomial $x^2 - x + 3 \equiv x^2 + x + 1 \pmod 2$, irreducible over $\mathbb{F}_2$, so $2$ is inert, $\pi = 2$, $q = 4$, $e = 1$, units $\pm 1$, $\kappa = v_2(-2) = 1$. The window is $(4^k, 4^{k+1/3})$, commuting maps exist from modulus $4$, and a map has at most two trivial cycles.
4. $\mathbb{Z}[\sqrt 2]$: the units $\pm(1 + \sqrt 2)^n$ form an infinite group, $2 = (\sqrt 2)^2$ ramifies, $e = 2$. For every $k$ the kernel of $U \to (R/P^k)^\times$ has finite index and so contains a unit $u \ne 1$, so no modulus admits a map commuting with all units; commuting with $\pm 1$ alone is possible from $k > e$, that is from $(\sqrt 2)^3$. The norm is not bounded below away from units ($N = \pm 1$ on every unit), so the definition of a trivial cycle and the argument below the window, which uses finitely many elements of bounded size, need both real embeddings.

These are structural facts. What the maps in these rings do (whether the modulus-$\pi$ maps of $\mathbb{Z}[\sqrt{-2}]$ are Collatz maps, whether the delay of commuting Collatz maps recurs) is untested.
