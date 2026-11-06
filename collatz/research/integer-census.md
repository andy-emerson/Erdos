# Symmetric Collatz rules: when they exist, and the integer census

Working notes accompanying `collatz/papers/gaussian-collatz-2.md` (Sections 3 to 5). Nothing here is in the paper yet. Scripts and logs are in this directory and in `logs/`. Section 8 lists what is proven, what is computed, and to what bound.

## 1. Summary

1. A rule symmetric under a finite unit group $U$ exists at level $k$ exactly when no unit $u \ne 1$ is congruent to $1$ modulo $P^k$, that is, $k > \max_{u \ne 1} v_P(u-1)$. This holds for every odd multiplier as soon as it holds for one (Theorem 1). The thresholds are $k \ge 2$ for $\mathbb{Z}$, $k \ge 3$ for $\mathbb{Z}[i]$ and $k \ge 2$ for the Eisenstein integers $\mathbb{Z}[\omega]$. With an infinite unit group no level works.
2. For a residue field of size $q$ the Collatz-like window is $q^k < N(\alpha) < q^{k + 1/(q-1)}$. For $\mathbb{Z}$ and $\mathbb{Z}[i]$ ($q = 2$) this is the paper's $(2^k, 2^{k+1})$. For $\mathbb{Z}[\omega]$ ($q = 4$) it is $(4^k, 4^{k+1/3})$, not $(4^k, 4^{k+1})$.
3. In $\mathbb{Z}$ the minimal symmetric digit set at level $k \ge 2$ is unique: the odd $d$ with $|d| < 2^{k-1}$. A fixed point $x > 0$ of the resulting rule is exactly an odd $x < 2^{k-1}/\delta$, where $\delta$ is the distance from $m$ to $\{2^k, 2^{k+1}\}$ (Proposition 4). So every rule with $m/2^k < 7/6$ or $m/2^k > 11/6$ has the second fixed point $3$. That is a third of each window, including both ends.
4. Integer census, levels $2$ to $10$: there are $1022$ rules. $251$ have a single cycle up to sign, and for each of these the drop-below-start check covers every odd $|x| \le 10^7$. Every other rule has at least two cycles, each checked independently in exact arithmetic or given by Proposition 4. By level the counts are $1, 0, 2, 4, 5, 15, 30, 67, 127$ out of $2, 4, 8, \ldots, 512$. No orbit appeared to diverge.
5. The single-cycle rules lie in $m/2^k \in [1.193, 1.832]$, inside the interval $(7/6, 11/6)$ that Proposition 4 allows. Inside the window they are missing near the resonances $m^n \approx 2^V$, most visibly around $m \approx \sqrt 2 \cdot 2^k$. The cycle counts there match the paper's heuristic count to within about $15\%$.
6. Compared with $\mathbb{Z}[i]$: the first single-cycle rule comes at the first symmetric level in $\mathbb{Z}$ (level $2$) and at the second in $\mathbb{Z}[i]$ (level $4$). In both rings it is the largest multiplier at its level ($7x \pm 1$ and $H$), and the next level has none ($\mathbb{Z}$ level $3$, $\mathbb{Z}[i]$ level $5$). In $\mathbb{Z}$ the smallest multiplier $2^k + 1$ is never single-cycle for $k \ge 3$. In $\mathbb{Z}[i]$ it is at levels $6$ and $7$. Lemma 3 explains the difference.
7. In $\mathbb{Z}[\omega]$ the first symmetric level, $2$, already has a single-cycle rule, $\alpha = 5 + 2\omega$ of norm $19$, and it is the smallest multiplier there. The drop-below-start check covers every odd $z$ with $N(z) \le 10^6$. Level $3$ has none among $32$ rules at $N(z) \le 10^5$.

## 2. When symmetric rules exist

### 2.1 Setting

Let $R$ be a Dedekind domain, $P$ a nonzero prime ideal of $R$ with finite residue field of size $q$, and $U$ a finite group of units of $R$. An element $z$ is odd if $z \notin P$. Fix an odd $\alpha$ and a level $k \ge 0$. A rule at level $k$ is a function $d$ on odd elements with these properties:

1. $d(z)$ is odd;
2. $d(z)$ depends only on the class of $z$ modulo $P^k$;
3. $\alpha z + d(z) \in P^k$.

The rule is $U$-symmetric if $d(uz) = u\,d(z)$ for every $u \in U$ and every odd $z$. Note that $uz$ is odd because $u$ is a unit.

For $\mathbb{Z}$, $\mathbb{Z}[i]$ and $\mathbb{Z}[\omega]$ with their full unit groups and $P = (2)$, $(1+i)$, $(2)$, this is the setting of the paper, and $U$-symmetry is the condition $d(uz) = u\,d(z)$ of Section 3.2 of the paper. Write $\kappa = \max_{u \in U, u \ne 1} v_P(u - 1)$, with $\kappa = -1$ if $U$ is trivial. For $u \ne 1$ the value $v_P(u-1)$ is finite.

### 2.2 The theorem

*Theorem 1.* The following are equivalent.

1. For some odd $\alpha$ there is a $U$-symmetric rule at level $k$.
2. For every odd $\alpha$ there is a $U$-symmetric rule at level $k$.
3. No $u \in U$ with $u \ne 1$ satisfies $u \equiv 1 \pmod{P^k}$, that is, $k > \kappa$.
4. $U$ acts freely on the odd classes modulo $P^k$.

*Proof.* (3) and (4) are equivalent. For odd $z$ and $u \in U$, $uz \equiv z \pmod{P^k}$ means $(u-1)z \in P^k$. Since $z \notin P$, $v_P((u-1)z) = v_P(u-1)$, so this holds exactly when $u - 1 \in P^k$. If $U$ is trivial, both conditions hold at every level.

(1) implies (3). Suppose $u \ne 1$ and $u \equiv 1 \pmod{P^k}$, and let $d$ be a $U$-symmetric rule. Take any odd $z$, for instance $z = 1$. Then $uz \equiv z \pmod{P^k}$, so $d(uz) = d(z)$, because the digit depends only on the class. Symmetry gives $d(uz) = u\,d(z)$. Hence $(1-u)\,d(z) = 0$. Since $R$ is a domain and $u \ne 1$, this forces $d(z) = 0$, which lies in $P$ and is not odd. This is a contradiction.

(4) implies (2). Fix an odd $\alpha$. If $U$ is trivial, symmetry is no condition, and we may take any element of the class of $-\alpha z$ as $d(z)$ (for $k = 0$ take $d = 1$). Otherwise $k \ge 1$, by the equivalence of (3) and (4) applied with $P^0 = R$. The odd classes modulo $P^k$ form a finite set on which $U$ acts freely, so they fall into orbits of exactly $|U|$ classes. Choose one class $c$ in each orbit, an element $r \in c$, and an element $\delta_c \in R$ with $\delta_c \equiv -\alpha r \pmod{P^k}$. Then $\delta_c$ is odd: since $k \ge 1$, $\delta_c \equiv -\alpha r \pmod P$, and $\alpha, r \notin P$ with $P$ prime. Every odd class is $uc$ for exactly one orbit representative $c$ and, by freeness, exactly one $u \in U$. Define $d(z) = u\,\delta_c$ for $z$ in the class $uc$. Then:

1. $d(z)$ depends only on the class of $z$;
2. $d(z)$ is odd, as a unit times an odd element;
3. for $z \equiv ur$, $\alpha z + d(z) \equiv u(\alpha r + \delta_c) \equiv 0 \pmod{P^k}$;
4. for $w \in U$, $wz$ lies in the class $(wu)c$, so $d(wz) = wu\,\delta_c = w\,d(z)$.

(2) implies (1) trivially. $\square$

The proof shows more. Up to the choice of $\delta_c$ inside its class, a symmetric rule is the same as a choice of one element in one class of each orbit. If norms are invariant under $U$, which holds in the three rings here since units have norm $1$, a minimal symmetric digit set is obtained by taking an element of least norm in one class of each orbit. The number of minimal symmetric digit sets is then the product, over the orbits, of the number of least-norm elements in a class of that orbit. This is how the counts of Section 4.4 of the paper (one, two, one, four, one at levels $3$ to $7$) arise.

### 2.3 Thresholds

Since $-1 \in U$ in each case and $P$ lies above $2$, $v_P(-1-1) = v_P(2) = e$, the ramification index of $P$ over $2$. So $\kappa \ge e$ whenever $-1 \in U$.

1. $\mathbb{Z}$, $P = (2)$, $U = \{\pm 1\}$: $v_2(-2) = 1$, so $\kappa = 1$ and symmetric rules exist exactly for $k \ge 2$. This recovers Section 3.6 of the paper.
2. $\mathbb{Z}[i]$, $P = (1+i)$, $U = \{\pm 1, \pm i\}$: $v(-2) = 2$, and $i - 1 = i(1+i)$ and $-i - 1 = -(1+i)$ have $v = 1$. So $\kappa = 2$, and symmetric rules exist exactly for $k \ge 3$. This is Proposition 2 of the paper.
3. $\mathbb{Z}[\omega]$, $\omega = e^{2\pi i/3}$, $P = (2)$ (inert, $q = 4$), $U = \{\pm 1, \pm\omega, \pm\omega^2\}$: $v_2(-2) = 1$. The element $\omega - 1$ has norm $3$, so it is odd, and so is $\omega^2 - 1 = (\omega - 1)(\omega + 1) = -\omega^2(\omega - 1)$. The elements $-\omega - 1 = \omega^2$ and $-\omega^2 - 1 = \omega$ are units. So $\kappa = 1$, and symmetric rules exist exactly for $k \ge 2$.
4. Any imaginary quadratic field other than $\mathbb{Q}(i)$ and $\mathbb{Q}(\sqrt{-3})$ has $U = \{\pm 1\}$, and $\kappa = e$. So the threshold is $k \ge 2$ when $2$ is unramified and $k \ge 3$ when $2$ ramifies.
5. Infinite unit groups, as in real quadratic fields. For every $k$ the kernel of $U \to (R/P^k)^\times$ has finite index in the infinite group $U$, so it contains some $u \ne 1$. Hence no level carries a rule symmetric under all units. Symmetry under the torsion subgroup $\{\pm 1\}$ alone is possible from $k > e$.

### 2.4 The window for a general residue field

Let $P = (\pi)$ be principal with $N(\pi) = q$, as in the three rings. Dividing by $\pi$ divides the norm by $q$. As $z$ runs over the odd classes modulo $P^{k+j}$ lying over one class modulo $P^k$, the value $\alpha z + d(z)$ runs once over the $q^j$ classes of $P^k$ modulo $P^{k+j}$, since $z \mapsto \alpha z + d$ is a bijection there. So in density $P(v - k \ge j) = q^{-j}$, the mean of $v - k$ is $\sum_{j \ge 1} q^{-j} = 1/(q-1)$, and the mean change of $\ln N$ per odd step is
$$\ln N(\alpha) - \left(k + \tfrac{1}{q-1}\right)\ln q.$$
A rule expands in the worst case exactly when $N(\alpha) > q^k$, so the Collatz-like window is
$$q^k < N(\alpha) < q^{k + 1/(q-1)}.$$
For $q = 2$ this is $(2^k, 2^{k+1})$, as in the paper. For $\mathbb{Z}[\omega]$, $q = 4$, it is $(4^k, 4^{k+1/3}) = (4^k, 2^{2k + 2/3})$: $(16, 25.40)$ at level $2$ and $(64, 101.6)$ at level $3$. The naive window $(4^k, 4^{k+1})$ would admit $23$ multipliers up to units at level $2$ instead of $5$, and all but those $5$ expand on average.

## 3. Integer rules: definitions and structure

### 3.1 The rules

On $\mathbb{Z}$, at level $k$ with odd $m$, $2^k < m < 2^{k+1}$, the accelerated map is $S(x) = (mx + d(x))/2^v$, where $d(x) \equiv -mx \pmod{2^k}$ is odd and $2^v$ is the full power of $2$. Two digit sets are used:

1. sym: $D_k = \{d \text{ odd} : |d| < 2^{k-1}\}$, the minimal symmetric set (Lemma 2);
2. pos: $\{1, 3, \ldots, 2^k - 1\}$, the least positive residues, as in $3x + 1$ (level $1$, $m = 3$).

The window $(2^k, 2^{k+1})$ holds $2^{k-1}$ odd multipliers, so levels $2$ to $10$ hold $1022$ rules of each kind.

*Lemma 2.* For $k \ge 2$, $D_k$ is the unique minimal symmetric digit set at level $k$. It is also the unique minimal digit set without the symmetry condition. Under the sym rule, $S(1) = 1$ for every $m$ in the window.

*Proof.* The interval $(-2^{k-1}, 2^{k-1})$ has length $2^k$ and even endpoints, so each odd class modulo $2^k$ has exactly one element in it. That element has absolute value less than $2^{k-1}$, while every other element of the class has absolute value greater than $2^{k-1}$. So each class has a unique element of least absolute value, and these elements form $D_k$, which is closed under negation. For $S(1)$, let $d$ be the digit of $1$. Then $m + d \equiv 0 \pmod{2^k}$ and $2^k - 2^{k-1} < m + d < 2^{k+1} + 2^{k-1}$, so $m + d$ is $2^k$ or $2^{k+1}$, and $S(1) = 1$. $\square$

So every sym rule has the fixed point $1$, and a sym rule with a single cycle up to sign is one in which every orbit reaches $\pm 1$.

### 3.2 Fixed points

*Lemma 3 (fixed classes, any of the three rings).* Let a symmetric rule have digit set $D$, with one element in each odd class, closed under $U$, and let $P = (\pi)$. An odd $z$ satisfies $S(z) = \varepsilon z$ with $\varepsilon \in U$ and exponent $v$ if and only if $v \ge k$ and $(\varepsilon\pi^v - \alpha)z \in D$.

*Proof.* If $S(z) = \varepsilon z$ with exponent $v$, then $\alpha z + d(z) = \varepsilon\pi^v z$, so $d(z) = (\varepsilon\pi^v - \alpha)z$, and $v \ge k$ by construction. Conversely, put $\delta = (\varepsilon\pi^v - \alpha)z \in D$. Since $v \ge k$, $\delta \equiv -\alpha z \pmod{\pi^k}$. So $\delta$ is the element of $D$ in the class of $-\alpha z$, that is, $d(z) = \delta$. Then $\alpha z + d(z) = \varepsilon\pi^v z$, and since $z$ is odd, $v$ is the exact exponent and $S(z) = \varepsilon z$. $\square$

*Proposition 4 (integers).* Let $2^k < m < 2^{k+1}$ with $m$ odd and $k \ge 2$, and let $\delta = \min(m - 2^k, 2^{k+1} - m)$.

1. Under the sym rule, an odd $x > 0$ is a fixed point exactly when $x < 2^{k-1}/\delta$, and no odd $x$ has $S(x) = -x$. The number of fixed points up to sign is $\lfloor (2^{k-1}/\delta + 1)/2 \rfloor$, and it is at least $2$ exactly when $\delta < 2^{k-1}/3$, that is, when $m < \frac{7}{6}2^k$ or $m > \frac{11}{6}2^k$.
2. In particular $m = 2^k + 1$ and $m = 2^{k+1} - 1$ have $2^{k-2}$ fixed points up to sign, namely every positive digit.
3. Under the pos rule, $1$ and $-1$ are fixed points for every $m$. A fixed point $x > 0$ is exactly an odd $x < 2^k/(2^{k+1} - m)$, so $3$ is a fixed point exactly when $m > \frac{5}{3}2^k$. A fixed point $x < 0$ is exactly an odd $|x| < 2^k/(m - 2^k)$.

*Proof.* (1) By Lemma 3 with $\pi = 2$, $x$ is fixed with exponent $v \ge k$ and sign $\varepsilon$ exactly when $|(\varepsilon 2^v - m)x| < 2^{k-1}$. With $\varepsilon = -1$, $|{-2^v} - m| > 2^{k-1}$, which is impossible. With $\varepsilon = 1$, $v \ge k + 2$ gives $2^v - m > 2^{k+1}$, also impossible. The values $v = k$ and $v = k+1$ give $|2^v - m| = m - 2^k$ and $2^{k+1} - m$. Exactly one of these is less than $2^{k-1}$, since $m \ne 3 \cdot 2^{k-1}$ ($m$ is odd), and the other is greater. So the condition is $\delta x < 2^{k-1}$. Since $\delta$ is odd and $2^{k-1}$ is even, $2^{k-1}/\delta$ is not an odd integer, and the count of odd $x$ below it is $\lfloor (2^{k-1}/\delta + 1)/2 \rfloor$. (2) is $\delta = 1$. (3) The same argument applies with digits in $(0, 2^k)$: $(2^v - m)x$ must lie in $(0, 2^k)$, which for $x > 0$ forces $v = k + 1$ and $(2^{k+1} - m)x < 2^k$, and for $x < 0$ forces $v = k$ and $(m - 2^k)|x| < 2^k$. Both hold for $|x| = 1$, since $m - 2^k$ and $2^{k+1} - m$ are less than $2^k$. $\square$

So no pos rule has a single cycle on all of $\mathbb{Z}$, just as $3x + 1$ has the cycles through $1$ and $-1$. For pos rules the natural question is the one for $3x + 1$: a single cycle on the positive integers, which the rule preserves.

The fixed points predicted by Proposition 4(1) agree exactly with the cycles of odd length $1$ in the census for all $1016$ rules run (`zsummary.py`).

## 4. The integer census

### 4.1 Method

For each level $k = 2, \ldots, 10$ and each odd $m$ in the window, `zcz census sym` follows every odd start $1 \le x \le 10^6$ to its cycle. By symmetry, the orbit of $-x$ is the negative of the orbit of $x$, so positive starts suffice. The program memoizes on $[-10^6, 10^6]$, uses Brent's method for orbits that stay outside that range, and groups cycles up to sign by least absolute value. Arithmetic is exact: $128$-bit integers, with a multi-word fallback for every excursion above $2^{100}$. An orbit counts as escaping only above $2^{2000}$. The step cap is $10^5$ odd steps, followed by up to $10^6$ more under Brent.

Two independent checks were run. `zverify.py` iterates two reported cycles of every multi-cycle rule in Python big integers and confirms that they are cycles of the reported length and are not equal or negatives of each other. The output of the multi-word code was also compared with a pure Python census for $m = 31$ at level $4$ (sym, $|x| \le 10^5$, and pos, $|x| \le 2 \cdot 10^4$), with identical cycles and basins.

For every single-cycle candidate, `zcz drop sym` checks that each odd $x$ with $3 \le x \le 10^7$ reaches some $|y| < x$, including excursions above $2^{100}$, followed exactly. With $S(1) = 1$ and $S(-x) = -S(x)$, induction on $|x|$ then shows that every odd $x$ with $|x| \le 10^7$ reaches $\pm 1$.

The eight rules $2033 \le m \le 2047$ at level $10$ sit at the top of the window, with drift between $-0.0073$ and $-0.0005$ per odd step. Their orbits climb past $2^{1000}$, and a census at $10^6$ was too slow to run. All of them have $m > \frac{11}{6} 2^{10}$, so Proposition 4 gives each at least two fixed points. $m = 2033$ and $2035$ were also run at $|x| \le 10^5$, and their cycles are exactly the $17$ and $20$ fixed points of Proposition 4. One start for $2033$ and $26$ for $2035$ hit the step cap.

### 4.2 Results

| $k$ | rules | single cycle | first single-cycle $m$ | smallest $m$: cycles | largest $m$: cycles | excluded by Prop. 4 | most cycles (rule) | checked to |
|---|---|---|---|---|---|---|---|---|
| 2 | 2 | 1 | 7 (largest) | 5: 2 | 7: 1 | 0 | 2 ($m = 5$) | $10^6$; drop $10^7$ |
| 3 | 4 | 0 | none | 9: 2 | 15: 3 | 2 | 3 ($m = 15$) | $10^6$ |
| 4 | 8 | 2 | 21 | 17: 4 | 31: 7 | 2 | 7 ($m = 31$) | $10^6$; drop $10^7$ |
| 5 | 16 | 4 | 41 | 33: 8 | 63: 12 | 6 | 12 ($m = 63$) | same |
| 6 | 32 | 5 | 77 | 65: 16 | 127: 16 | 10 | 50 ($m = 97$) | same |
| 7 | 64 | 15 | 153 | 129: 32 | 255: 32 | 22 | 294 ($m = 181$) | same |
| 8 | 128 | 30 | 307 | 257: 64 | 511: 64 | 42 | 64 ($m = 257, 511$) | same |
| 9 | 256 | 67 | 611 | 513: 128 | 1023: 128 | 86 | 128 ($m = 513, 1023$) | same |
| 10 | 512 | 127 | 1223 | 1025: 256 | 2047: 256 (Prop. 4) | 170 | 256 ($m = 1025, 2047$) | same; $m \ge 2033$ see 4.1 |

"Excluded by Prop. 4" counts the rules with $\delta < 2^{k-1}/3$, which provably have at least two cycles. In total $251$ of $1022$ rules have a single cycle up to sign. The full lists are in `logs/zsummary_sym.txt`. Level $2$ has $7x \pm 1$, level $4$ has $m = 21$ and $25$, and level $5$ has $m = 41, 43, 49, 53$.

The level-$3$ rules have these cycles, all exhibited in exact arithmetic: $m = 9$ has the fixed points $1$ and $3$; $m = 11$ has $1$ and a 2-cycle through $5$; $m = 13$ has $1$ and a cycle of $10$ odd steps through $151$; $m = 15$ has $1$, $3$ and a cycle of $43$ odd steps through $89$.

Apparent divergence. None was found. At first, with an escape threshold of $2^{100}$, many rules near the top of each window seemed to have escaping starts, for example $10^4$ or more for $m = 61, 63$ at level $5$. Every such orbit returns when followed exactly, and with the multi-word fallback no start passed $2^{2000}$ at any level. The peaks reach $2^{1322}$, for $m = 255$ at level $7$. Eleven rules have starts that hit the step cap: $m = 127$ (level $6$), $255$ (level $7$), $509$ and $511$ (level $8$), $1017$ to $1023$ (level $9$), and $2031$, $2033$, $2035$ (level $10$). All of them lie in the range of Proposition 4, so this does not affect the count of single-cycle rules. Their drift lies between $-0.0083$ and $-0.0010$ per odd step, and long excursions are expected for a random walk with so little drift. Integers have no analogue of the invariant lines that carry the divergence in $\mathbb{Z}[i]$: the line is the whole ring, and every integer rule in the window has negative drift.

### 4.3 Where the single-cycle rules lie

The single-cycle multipliers satisfy $m/2^k \in [1.193, 1.832]$ at levels $7$ to $10$, inside the interval $(7/6, 11/6) = (1.167, 1.833)$ left by Proposition 4. Their drift $\ln m - (k+1)\ln 2$ lies in $[-0.5164, -0.0877]$. Proposition 4 bounds the drift of any single-cycle sym rule to $(\ln(7/12), \ln(11/12)) = (-0.539, -0.087)$, and the observed range nearly fills it. Grouped by tenths of the window, $m/2^k - 1 \in [i/10, (i+1)/10)$, over all levels (the $1016$ rules run; the six left out are in tenth $9$):

| tenth | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 |
|---|---|---|---|---|---|---|---|---|---|---|
| single-cycle / rules | 0/102 | 7/102 | 45/103 | 46/102 | 0/102 | 51/102 | 40/102 | 47/103 | 15/102 | 0/96 |

Tenths $0$ and $9$ are mostly excluded by Proposition 4. Tenth $4$, around $\sqrt 2 \approx 1.414$, is empty for a different reason. There $m^2 \approx 2^{2k+1}$, and the rules have many 2-cycles with total exponent $2k+1$. At level $10$ every rule with $1391 \le m \le 1469$ has at least one, rising to $53$ at $m = 1449$, where $1449^2/2^{21} = 1.0012$. Thinner gaps sit near $2^{1/3}$, $2^{2/3}$, $2^{1/4}$ and $2^{3/4}$. At level $10$ the single-cycle list skips from $1255$ to $1309$ around $2^{1/3} \cdot 1024 = 1290$, and from $1611$ to $1653$ around $2^{2/3} \cdot 1024 = 1625$. These are the near-coincidences of Section 5.6 of the paper.

### 4.4 The heuristic count at the resonances

The paper's count, transcribed to $\mathbb{Z}$ with $|U| = 2$, is
$$E(n, V) = \frac{2^{(k-1)n}\binom{V - kn + n - 1}{n-1}}{2n\,|2^V - m^n|}.$$
It predicts the cycle counts at the resonances well (`zresonance.py`, `logs/zresonance.txt`):

| $k$ | $m$ | $n$, $V$ | $m^n/2^V$ | $n$-cycles found | $E$ | all cycles |
|---|---|---|---|---|---|---|
| 7 | 181 | 2, 15 | 0.999786 | 293 | 292.6 | 294 |
| 10 | 1449 | 2, 21 | 1.0012 | 53 | 53.5 | 54 |
| 10 | 1447 | 2, 21 | 0.9984 | 41 | 39.2 | 42 |
| 9 | 645 | 3, 28 | 0.999630 | 85 | 84.5 | 86 |
| 6 | 97 | 5, 33 | 0.999698 | 49 | 45.3 | 50 |
| 10 | 1505 | 9, 95 | 0.999951 | 92 | 89.9 | 97 |
| 8 | 363 | 2, 17 | 1.0053 | 12 | 11.8 | 13 |

Across the $\sqrt 2$ resonance at levels $9$ and $10$ the count of 2-cycles follows $E(2, 2k+1)$ rule by rule, typically within $15\%$ (for example $16$ against $14.4$ at $m = 1445$, and $4$ against $4.1$ at $m = 1437$). The integer census therefore confirms in one dimension what Section 5.6 of the paper found for the Gaussian rules with many cycles. The cycles sit at the near-coincidences of $m^n$ with a power of $2$, in about the expected numbers.

## 5. Comparison with the Gaussian census

| | $\mathbb{Z}$ | $\mathbb{Z}[i]$ (paper, Section 5) |
|---|---|---|
| first symmetric level | 2 | 3 |
| single-cycle rules by level | $k = 2{:}\ 1/2$, $3{:}\ 0/4$, $4{:}\ 2/8$, $5{:}\ 4/16$, $6{:}\ 5/32$, $7{:}\ 15/64$, $8{:}\ 30/128$, $9{:}\ 67/256$, $10{:}\ 127/512$ | $k = 3{:}\ 0/3$, $4{:}\ 2/14$, $5{:}\ 0/11$, $6{:}\ 22/112$, $7{:}\ 8/46$ |
| first level with a single-cycle rule | 2 (the first symmetric level) | 4 (the second) |
| levels with none, after the first | 3 | 5 |
| first single-cycle rule | $7x \pm 1$, largest $m$ at level 2 | $H$, $\alpha = 5 + 2i$, largest norm at level 4 |
| smallest multiplier single-cycle? | never for $k \ge 3$ (Prop. 4) | yes at levels 6 ($7 \pm 4i$, norm 65) and 7 ($11 \pm 4i$, norm 137) |
| drift of single-cycle rules | $-0.516$ to $-0.088$, confined by Prop. 4 to $(-0.539, -0.087)$ | $H$: $\ln(29/32) = -0.098$; level 6: $-0.68$ to $-0.024$ ($11 \pm 2i$); level 7: $-0.63$ to $-0.28$ |
| apparent divergence | none | real multipliers $5$, $9$, $11$, on invariant lines |

The proportion of single-cycle rules settles near a quarter in $\mathbb{Z}$, from level $7$ on ($23\%$, $23\%$, $26\%$, $25\%$). The Gaussian proportions at levels $4$ to $7$ are $14\%$, $0\%$, $20\%$, $17\%$. Both rings have a gap after the first single-cycle level: $\mathbb{Z}$ level $3$, $\mathbb{Z}[i]$ level $5$.

Lemma 3 explains why the smallest multiplier behaves differently. In $\mathbb{Z}$ the closeness of $m$ to $2^k$ or $2^{k+1}$ is the quantity $\delta$ of Proposition 4. A multiplier near either end of the window has many small fixed points. In $\mathbb{Z}[i]$ what matters is the distance from $\alpha$ to the eight points $\varepsilon(1+i)^k$ and $\varepsilon(1+i)^{k+1}$, and a norm near $2^k$ does not make it small.

1. Of the two norm-$65$ pairs at level $6$, $8 \pm i$ lies at distance $1$ from $8 = i(1+i)^6$. So $8 - \alpha = \mp i$ is a unit, every digit is a fixed class by Lemma 3, and each of its four digit sets has at least $8$ fixed classes. This is consistent with the paper's report of four rules with $8$ cycles at level $6$. The other pair, $7 \pm 4i$, lies at distance $\sqrt{17}$ from that point and has a single cycle.
2. For $11 + 4i$ at level $7$, $(8 + 8i) - (11 + 4i) = -3 + 4i$, an associate of $3 - 4i$, which lies in the level-$7$ minimal set. Only $z = 1$ passes Lemma 3, since $25\,N(z) \le 49$ forces $N(z) = 1$. So $1$ is the only fixed class, consistent with the single cycle found.
3. The structural fixed points of the paper fit the same pattern: $\alpha = 9$ at level $6$ ($8 - 9 = -1$) and $\alpha = 15$ at level $7$ ($16 - 15 = 1$).
4. For $H$, $(4 + 4i) - (5 + 2i) = -1 + 2i = i(2 + i)$ lies in the digit set, giving the fixed class of $1$ with exponent $5$, and no other $z$ passes.

The same lemma explains why $7x \pm 1$ is the first integer single-cycle rule while $5x \pm 1$ is not. Both have $\delta = 1$, so the fixed point condition gives only $x = 1$. But $5x \pm 1$ has the 3-cycle through $7$, and $7x \pm 1$ has none.

## 6. Non-symmetric rules for contrast

For the pos digit set (least positive residues), Proposition 4(3) proves that $1$ and $-1$ are both fixed, so no pos rule has a single cycle on $\mathbb{Z}$. It also proves that every pos rule with $m > \frac{5}{3} 2^k$ has the positive fixed point $3$. The rules with $m < \frac{5}{3}2^k$ were run over all odd $|x| \le 10^5$ (`logs/zcensus_pos_k2-10_B100000_mlt5over3.txt`, step cap $10^4$), with no unresolved start. Rules with a single cycle on the positive integers:

| $k$ | rules tested ($m < \frac53 2^k$) | single positive cycle | single negative cycle |
|---|---|---|---|
| 2 | 1 of 2 | 0 | 0 |
| 3 | 3 of 4 | 1 ($m = 9$) | 1 ($m = 11$) |
| 4 | 5 of 8 | 2 ($17, 23$) | 0 |
| 5 | 11 of 16 | 3 ($33, 37, 51$) | 2 |
| 6 | 21 of 32 | 5 | 1 |
| 7 | 43 of 64 | 15 | 6 |
| 8 | 85 of 128 | 33 | 9 |
| 9 | 171 of 256 | 55 | 20 |
| 10 | 341 of 512 | 112 | 36 |

For the pos rules the picture reverses. The smallest multiplier $m = 2^k + 1$ has a single positive cycle at every level from $3$ to $10$ (to $10^5$). By Proposition 4(3) its only positive fixed point is $1$, since $2^k/(2^k - 1) < 3$. The single-cycle rules also crowd toward the bottom of the window. The symmetric rules spread the digits over both signs, and this is what lets the small multipliers have fixed points at $\pm 3, \pm 5, \ldots$.

## 7. The Eisenstein integers

Notation: $z = a + b\omega$, $\omega^2 = -1 - \omega$, $N(z) = a^2 - ab + b^2$. The element $z$ is odd when $a$ and $b$ are not both even. Each class of associates has exactly one member with $0 \le b < a$, the sector $0 \le \arg z < 60°$, which is used as the canonical representative. Script: `zeis.py`.

Level $2$, the first symmetric level by Theorem 1. There are $12$ odd classes modulo $4$, in $2$ orbits of $6$. Each class has a unique element of least norm: the units (norm $1$), and the six associates of $1 - \omega$ (norm $3$). So the minimal symmetric digit set is unique, $D = U \cup (1 - \omega)U$. The window $(16, 25.40)$ contains the odd norms $19$, $21$ and $25$, giving $5$ multipliers up to units: $5 + 2\omega$ and its conjugate $5 + 3\omega$ (norm $19$), $5 + \omega$ and $5 + 4\omega$ (norm $21$), and $5$. Up to conjugation that is $3$ rules.

| $\alpha$ | $N$ | drift | cycles up to units, $N(z) \le 10^5$ |
|---|---|---|---|
| $5 + 2\omega$ (and conjugate) | 19 | $-0.290$ | 1: the fixed class of $1$ |
| $5 + \omega$ (and conjugate) | 21 | $-0.190$ | 3: $1$, $2 + \omega$ (fixed), and $7 + 3\omega$ (11 odd steps) |
| $5$ | 25 | $-0.016$ | 84 found and 9104 unresolved starts |

For $5 + 2\omega$ and $5 + 3\omega$, every odd $z$ with $1 < N(z) \le 10^6$ reaches a smaller norm (`zeis.py drop`, in exact arithmetic, one representative per class of associates). So every odd $z$ with $N(z) \le 10^6$ reaches a unit. Lemma 3 accounts for the fixed classes:

1. $4 - (5 + \omega) = \omega^2$ is a unit, so both digit classes are fixed for $5 + \omega$;
2. likewise $4 - 5 = -1$ for $\alpha = 5$;
3. for $5 + 2\omega$, $4 - \alpha = -1 - 2\omega$ has norm $3$ and lies in $D$, so $1$ is fixed and nothing else is.

For the real multiplier $5$ the picture of the Gaussian real multipliers recurs. Of $139$ random odd starts with $|a|, |b| \le 300$, $59$ passed norm $2^{400}$, and every one ended on a line at distance one from a unit line: $b = 1$ ($33$ starts) or $a - b = 1$ ($26$), in the canonical sector (`zeisline.py`, `logs/zeis_lines.txt`).

Level $3$. There are $48$ odd classes modulo $8$, in $8$ orbits. One orbit, that of the class of $2 + 5\omega$, has two elements of least norm $19$, namely $2 - 3\omega$ and $2 + 5\omega$, so there are two minimal digit sets, and they are conjugate. The window $(64, 101.6)$ gives $16$ multipliers up to units, with norms $67, 73, 75, 79, 81, 91, 93, 97$, so $32$ rules. At $N(z) \le 10^5$ none has a single cycle. The counts are $2$ or $3$ for most rules, $4$ for two, $8$ for both rules with $10 + 5\omega$ (norm $75$), $10$ or $14$ for the norm-$73$ rules, and $42$ with $5829$ unresolved starts for the real $\alpha = 9$. For $\alpha = 9$, $45$ of $139$ random starts passed norm $2^{400}$, all on the lines $b = 1$ or $a - b = 1$. Every rule has a cycle through $1$ (the fixed class of $1$, or, for $11 + 5\omega$ and $11 + 6\omega$ of norm $91$ with one of the two digit sets each, a cycle of two odd steps through $1$) and at least one further cycle, found in exact arithmetic (`logs/zeis_k3_B1e5.txt`).

So in $\mathbb{Z}[\omega]$ the first symmetric level already has a single-cycle rule, and it is the rule with the smallest multiplier. In the order of Section 3.5 of the paper, the first symmetric, minimal, Collatz-like Eisenstein rule with a single cycle is $\alpha = 5 + 2\omega$ (and its mirror image) at level $2$, up to $N(z) \le 10^6$. Its drift, $\ln 19 - \frac{7}{3}\ln 4 = -0.290$, is much more negative than that of $7x \pm 1$ ($-0.134$) or $H$ ($-0.098$).

## 8. What is proven and what is computed

Proven: Theorem 1 and its thresholds (Section 2.3), including the remark on infinite unit groups; the window of Section 2.4, as a density statement about valuations, which makes it a heuristic for orbits as in the paper; Lemma 2; Lemma 3; and Proposition 4. Proposition 4 gives, for every level $k \ge 3$, at least two cycles for every sym rule with $m/2^k < 7/6$ or $m/2^k > 11/6$, including both ends of every window, and for every pos rule on $\mathbb{Z}$.

Computer-assisted, resting on `zcz.c` and `zeis.py` as written:

1. *Proposition 6.* At levels $2$ to $10$, every symmetric minimal integer rule except the $251$ listed in `logs/zsummary_sym.txt` has at least two cycles up to sign. For $765$ rules two cycles were checked independently by `zverify.py`. The six rules $2037 \le m \le 2047$ at level $10$ are covered by Proposition 4.
2. Each of the $251$ rules sends every odd $x$ with $|x| \le 10^7$ to $\pm 1$ (drop-below-start, `logs/zcensus_sym_drop_1e7.txt`).
3. Census counts of cycles are over $|x| \le 10^6$ (levels $2$ to $10$, $m \le 2031$) or $|x| \le 10^5$ ($m = 2033, 2035$). They are lower bounds for rules with starts at the step cap, all of which lie in the range of Proposition 4.
4. Eisenstein: $5 + 2\omega$ and $5 + 3\omega$ send every odd $z$ with $N(z) \le 10^6$ to a unit. Every other symmetric minimal Collatz-like rule at levels $2$ and $3$ has at least two cycles up to units. Counts are at $N(z) \le 10^5$.
5. Pos rules: single positive cycle to $|x| \le 10^5$ only.

Observed but not proven: that the single-cycle rules really have one cycle (Conjecture 1 of the paper, for each of them); the absence of divergence on $\mathbb{Z}$; the gaps at the resonances and the agreement with $E(n, V)$, which is a heuristic; the divergence along lines for real Eisenstein multipliers. Integer rules of this kind may be in the literature on generalized Collatz maps (Matthews; Lagarias's bibliographies), which has not been searched for them.

## 9. Files

1. `zcz.c` (binary `zcz`): integer census and drop-below-start check, exact $128$-bit and multi-word arithmetic. `gcc -O2 -o zcz zcz.c -lm`.
2. `zcensus.py`: runs `zcz census` over levels and windows. `zsummary.py`: per-level summary and the check of Proposition 4. `zverify.py`: independent check of two cycles per rule. `zesc.py`: follows starts that once escaped past $2^{100}$, in Python big integers. `zresonance.py`: the heuristic count $E(n, V)$.
3. `zeis.py`: Eisenstein digit sets, multipliers, census and drop check. `zeisline.py`: escaping orbits of real Eisenstein multipliers.
4. Logs in `logs/`: `zcensus_sym_k2-10_B1000000.txt`, `zcensus_sym_k10top_B100000.txt`, `zcensus_sym_drop_1e7.txt`, `zsummary_sym.txt`, `zverify.txt`, `zresonance.txt`, `zcensus_pos_k2-10_B100000_mlt5over3.txt`, `zeis_k2_B1e5.txt`, `zeis_k2_drop.txt`, `zeis_k3_B1e5.txt`, `zeis_lines.txt`.
