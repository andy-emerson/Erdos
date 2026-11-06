# Collatz-type maps on real quadratic rings

Working notes, 6 November 2026. Nothing here is in the papers or the catalog. Vocabulary is that of `catalog.md`: map, Collatz map, trivial cycle, unit cycle, respects associates, commutes with the units, Barina-type map (multiplier $\alpha$, modulus $\pi^k$, digits by residue class modulo $\pi^k$, read literally on $R$), Collatz-type map (a map on a fundamental domain, $S_F(z) = \mathrm{rep}_F((\alpha z + d(z))/\pi^v)$), fundamental domain, modulus, window, embedding. This file continues Section 9 of `rings-test.md` (Z[√2]) and uses the transfer theorem and cycle lifts of `quotient.md` (A4 to A7).

## 1. Summary

1. The per-embedding window predicts the behaviour of Barina-type maps read literally on $R$. With $\ell_j(x) = \log_2|\sigma_j(x)|$ and $m = k + 1/(q-1)$, the drift in embedding $j$ is $D_j = \ell_j(\alpha) - m\,\ell_j(\pi)$, and $D_1 + D_2$ is the norm drift. In 144 runs over three rings (Z[√2] with $\pi = \sqrt2$, Z[√3] with $\pi = 1 + \sqrt3$, Z[(1+√5)/2] with $\pi = 2$ and $q = 4$), the measured slope of $\log_2|\sigma_j|$ matched $D_j$ (median error 0.02 bits, 90th percentile 0.14). Orbits escaped in the predicted embedding whenever $\max_j D_j > 0$, and no start escaped when both $D_j < 0$, in 143 of the 144 runs (Section 3.5). The exception is the rational multiplier 5 in Z[(1+√5)/2]. It sits in an invariant line on which every step has $v = k$, so the valuations are not distributed as the drift assumes (Section 3.6).
2. The norm window is the wrong criterion on $R$ in both directions. Multipliers inside it diverge in one embedding when every associate is unbalanced: in Z[√2] no associate of $1 + 2\sqrt2$ or $3 + \sqrt2$ works at $\sqrt2^2$, and none of $3 + 5\sqrt2$ at $\sqrt2^5$. Multipliers far below it diverge too: the unit $1 + \sqrt2$ at modulus $\sqrt2$ does. In Z[√3] even $\alpha = 1$ diverges, because $|\sigma_2(1+\sqrt3)| = 0.73 < 1$, so every division by $\pi$ expands $\sigma_2$. In Z[√3] no generator of the prime above 2 is balanced, and the window has to be stated in the log form of Section 3.1. Its lower end, and the below-the-window theorem W1, are then lost for the embedding where $|\sigma_j(\pi)| < 1$.
3. Units and the transfer theorem (Section 4, proved). Let $T$ respect associates, let $p$ be the period of $[1]$, and let $g = T^p$ restricted to $U$. Then $T$ is a Collatz map on $R$ iff $\bar T$ is a Collatz map on $R/U$ and every orbit of $g$ on $U$ is eventually periodic. For a map that commutes with the units, $g(u) = uw$ with $w = T^p(1)$. Units then lie on cycles iff $w = \pm1$: one unit per trivial cycle for $w = 1$ (infinitely many trivial cycles), the pair $\{u, -u\}$ for $w = -1$. They run along $2|j|$ endless walks with no periodic unit iff $w = \pm\varepsilon^j$ with $j \ne 0$. Rotating the multiplier by a unit $u$ multiplies $w$ by $u^p$, so some rotation lifts to a Collatz map on $R$ iff $p \mid j$. A free choice of lift always works.
4. Computed on the cone in all three rings: Collatz-type maps on $R/U$ exist at the first modulus with multipliers, and their natural lifts have $w = \pm1$ or $w = \pm\varepsilon^j$. In Z[√3], $w = \pm1$ never occurs on the cone representative, because $\pi^2 = 2\varepsilon$ puts a unit into every double division. A rotation cures this in 233 of 255 maps at $\pi^3$. Maps with $p \nmid j$, which no rotation cures, occur in all three rings. The theory was checked on every map and about 4000 starts per map, with no exception.
5. The quotient form of the Collatz property (every orbit enters $U$ and stays there) is, for maps that respect associates, the same as "$\bar T$ is a Collatz map on $R/U$ with $\bar T[1] = [1]$". It is satisfied by the lifts of the Collatz-type maps with $p = 1$. Examples are given in full, with fixed units, 2-cycles $\{u, -u\}$, and $2|j|$ endless walks $u \mapsto u\varepsilon^j$ for $|j| \le 3$. No residue-class map satisfies it. A residue-class map sends only finitely many units to units, by the finiteness of S-unit equations, and in the computations at most 36 of the 802 units $\pm\varepsilon^n$, $|n| \le 200$.
6. Barina-type maps commuting with $\pm1$ exist from $\pi^{\kappa+1}$ with $\kappa = v_\pi(2)$: $\sqrt2^3$, $(1+\sqrt3)^3$ and $4$. The first literal Collatz maps on $R$ found are at $\sqrt2^4$, with $\alpha = 1 + 3\sqrt2$ and $-1 + 3\sqrt2$ and tables commuting with $-1$ (four maps, each checked on every odd start with $|N| \le 2\cdot10^5$ in a box, and on units to $\varepsilon^{\pm40}$). They have 12 or 10 trivial cycles: runs of fixed units $\pm\varepsilon^n$, plus 5-cycles or 10-cycles through $\pm\varepsilon^{3}, \ldots, \pm\varepsilon^{5}$. None were found in Z[√3] up to $\pi^5$ or in Z[(1+√5)/2] up to 8 (sampled tables). At the first moduli every multiplier with an associate in the per-embedding window is forced, since $\pi^v - \alpha$ is a unit: $3 + 2\sqrt3 = \pi^2 - 1$, $3 = \sqrt2^4 - 1$, $7 = \sqrt2^6 - 1$, $5 = 2^2 + 1$, $3\varepsilon^2 = \pi^4 - \varepsilon^2$ in Z[√3]. Collatz-type maps on $R/U$ exist from the first modulus with multipliers in all three rings, so the gap between the two notions is two moduli or more.
7. A bug in `qring.py`, found on the way: `Ring.classrep(i, K)` returns $(x + yc, y)$, which is not in class $i$ when $yc \not\equiv 0 \bmod A$. Of the seven rings this affects only Z[i] at odd $K \ge 3$. Collatz-type and literal tables built through `classrep` there are mislabelled (a cone table for $3 + 2i$ at $(1+i)^3$ gives $v < 3$ on about half the starts). The commuting $5+2i$ tables happened to come out valid. `qring.py` has since been fixed and the affected Z[i] runs rechecked (`rings-test.md` Section 14). `realq.py` uses the correct representative $(x, y)$.

## 2. Setting

Rings (`realq.py`), elements $a + b\tau$ with $\tau^2 = t\tau - n$:

| ring | $\tau$ | $\pi$ | $q$ | $2$ | $\varepsilon$ | $N(\varepsilon)$ | $(\ell_1(\pi), \ell_2(\pi))$ | $\log_2\varepsilon$ |
|---|---|---|---|---|---|---|---|---|
| Z[√2] (Zr2) | $\sqrt2$ | $\sqrt2$ | 2 | $\pi^2$ | $1 + \sqrt2$ | $-1$ | $(0.5, 0.5)$ | 1.272 |
| Z[√3] (Zr3) | $\sqrt3$ | $1 + \sqrt3$ | 2 | $\pi^2\varepsilon^{-1}$ | $2 + \sqrt3$ | $+1$ | $(1.450, -0.450)$ | 1.900 |
| Z[(1+√5)/2] (Zg5) | $g = (1+\sqrt5)/2$ | $2$ (inert) | 4 | $\pi$ | $g$ | $-1$ | $(1, 1)$ | 0.694 |

$U = \{\pm\varepsilon^n\}$. "Odd" means not divisible by $\pi$. The cone $E = \{\sigma_1(z) > 0,\ 1 \le |\sigma_1(z)/\sigma_2(z)| < \varepsilon^2\}$ is a fundamental domain for $U$ in every ring, since multiplication by $\varepsilon$ multiplies $|\sigma_1/\sigma_2|$ by $\varepsilon^2$ ($|\sigma_2(\varepsilon)| = \varepsilon^{-1}$) and $-1$ changes the sign. Membership is exact: $|\sigma_1(z)| \ge |\sigma_2(z)|$ iff $b(2a + tb) \ge 0$, and the sign of $\sigma_1$ is decided by comparing squares. Digits are ordered by the Minkowski size $\sigma_1^2 + \sigma_2^2$, ties kept. The C engine `realfd.c` is `ringfd.c` with this cone made generic ($\varepsilon$ passed as the second unit); on Z[√2] it reproduces `ringfd.c` exactly (6 and 2 cone maps at $\sqrt2^2$, identical output lines). `realq.lg` computes $\ell_j$ for huge elements without cancellation (the larger embedding directly, the smaller as $\log_2|N| - \ell_{\text{larger}}$).

In Z[√3] no generator of the prime above 2 is balanced. Every generator is $(1 + \sqrt3)u$, with $|\sigma_1/\sigma_2| = \varepsilon^{\mathrm{odd}}$, so one embedding of $\pi$ has absolute value below 1. Equivalently $\pi^2/2$ is a unit that is not $\pm$ a square of a unit. This, not the norm of $\varepsilon$, is what separates Z[√3] from the other two rings below.

## 3. The per-embedding window

### 3.1 Statement

Let $K$ be a number field with archimedean places $\sigma_1, \ldots, \sigma_s$ (a complex place counted once, with $|\cdot|$ its usual absolute value). Let $\pi$ generate a prime above 2 with residue field of size $q$, and put $\ell_j(x) = \log_2|\sigma_j(x)|$ and $m = k + 1/(q-1)$. A map $(\alpha, k, d)$ is Collatz-like in embedding $j$ when it contracts there on average, $\ell_j(\alpha) < m\,\ell_j(\pi)$, and expands there in the worst case, $\ell_j(\alpha) > k\,\ell_j(\pi)$. The per-embedding window is the condition in every embedding:

$$|\sigma_j(\pi)|^k < |\sigma_j(\alpha)| < |\sigma_j(\pi)|^{k + 1/(q-1)} \quad \text{for every } j \text{ with } |\sigma_j(\pi)| > 1,$$
$$|\sigma_j(\alpha)| < |\sigma_j(\pi)|^{k + 1/(q-1)} \quad \text{for every } j \text{ with } |\sigma_j(\pi)| \le 1.$$

In an embedding with $|\sigma_j(\pi)| < 1$ the worst case is a large $v$, which always expands, so the lower inequality is vacuous; written as two-sided it would be an empty interval. With one embedding the condition is the norm window. For Z, $|\sigma(x)| = |x|$ and $q = 2$; for an imaginary quadratic ring, one complex place with $|\sigma(x)|^2 = N(x)$ and $|\pi|^2 = q$, so $|\pi|^k < |\alpha| < |\pi|^m$ is $q^k < N(\alpha) < q^m$. In general the product of the upper ends over all places (complex ones twice) is the upper end of the norm window, since $\prod|\sigma_j(\pi)| = q$. So the per-embedding window implies the upper half of the norm window, and the converse fails as soon as there are two places.

### 3.2 The drift per embedding

Let $z$ be odd with Haar measure on the odd elements of $R_\pi$. For $z$ in an odd class $c$ modulo $\pi^k$ the map $z \mapsto \alpha z + d(c)$ is an affine bijection from $c$ onto $\pi^k R_\pi$, so $v - k$ is geometric with $P(v - k \ge j) = q^{-j}$, $E[v] = m$ and $\mathrm{Var}(v) = q/(q-1)^2$ (W3). One odd step changes the log-embedding vector by

$$\ell(S z) - \ell(z) = \ell(\alpha) + \delta(z) - v\,\ell(\pi), \qquad \delta_j(z) = \log_2|1 + \sigma_j(d)/\sigma_j(\alpha z)|,$$

and $\delta_j \to 0$ when $|\sigma_j(z)| \gg |\sigma_j(d/\alpha)|$. So while both embeddings are large compared with the digits, the mean change is $D_j = \ell_j(\alpha) - m\,\ell_j(\pi)$, with $D_1 + D_2 = \log_2|N(\alpha)| - m\log_2 q$ the norm drift. As for W3, this is a statement about Haar measure; along an orbit it is the heuristic that the valuations are independent and geometric.

Two consequences of the form of the noise. The fluctuation of the step is $-(v - m)\,\ell(\pi)$, a multiple of the fixed vector $\ell(\pi)$. If $\pi$ is balanced ($\ell_1(\pi) = \ell_2(\pi)$, Z[√2] and Z[(1+√5)/2]), the log ratio $\ell_1 - \ell_2$ moves deterministically by $\ell_1(\alpha) - \ell_2(\alpha)$ per step. In Z[√3] the log ratio fluctuates as well, with standard deviation $\sqrt2\,\log_2\varepsilon = 2.69$ bits per step.

When one embedding expands ($D_1 > 0 > D_2$, say), $\sigma_2$ contracts until the digits dominate and then stays of the size of the digits, while $\sigma_1$ keeps growing at rate $D_1$. So $|N|$ grows at rate $D_1$ and the orbit diverges in $\sigma_1$. When both contract, both embeddings enter a bounded range, and an element of $R$ is determined by its two embeddings. The orbit is then in a finite set and is eventually periodic. So for maps read on $R$, the per-embedding window is the boundary between divergence and eventual periodicity, as the norm window is in rings with one place.

Associates. Multiplying $\alpha$ by $\varepsilon^r$ adds $r(\log_2\varepsilon, -\log_2\varepsilon)$ to $(D_1, D_2)$ and leaves the norm drift alone. With $D_1 + D_2 = -\beta < 0$, some associate is in the window iff the log ratio can be brought into $(-\beta, \beta)$. That interval has width $2\beta < 2/(q-1)$ against the period $2\log_2\varepsilon$ (2.54, 3.80 and 1.39 bits). So most classes have no associate in the window, and the ones that do are near $\pi^m$ in both embeddings.

### 3.3 Below the window

Proposition 3.1. Suppose $|\sigma_j(\pi)| > 1$ and $\rho_j = |\sigma_j(\alpha)|/|\sigma_j(\pi)|^k < 1$ for every $j$. Then every orbit of $S$ is eventually periodic.

Proof. With $\delta_j = \max_c |\sigma_j(d(c))|$ and $v \ge k$, $|\sigma_j(Sz)| \le (|\sigma_j(\alpha)||\sigma_j(z)| + \delta_j)/|\sigma_j(\pi)|^v \le \rho_j|\sigma_j(z)| + \delta_j/|\sigma_j(\pi)|^k$. So $|\sigma_j(S^n z)|$ is eventually below $R_j = 1 + \delta_j/(|\sigma_j(\pi)|^k(1 - \rho_j))$ for both $j$. The elements with $|\sigma_1| \le R_1$ and $|\sigma_2| \le R_2$ form a finite set, because $z \mapsto (\sigma_1 z, \sigma_2 z)$ embeds $R$ as a lattice. $\square$

This is W1 with one condition per embedding. It does not apply to Z[√3], where $|\sigma_2(\pi)| < 1$, and the hypothesis on the norm alone gives nothing on $R$: below the norm window, unbalanced associates diverge (Section 3.5).

### 3.4 Which classes have an associate in the window

`realbal.py`, `logs/realq_balanced.txt`. Classes of associates with $|N|$ in the norm window, and those with an associate in the per-embedding window (best associate listed):

| ring | $k = 1$ | 2 | 3 | 4 | 5 | 6 | 7 |
|---|---|---|---|---|---|---|---|
| Z[√2] | 0 of 0 | 0 of 2 | 1 of 1 ($3$) | 3 of 7 ($\pm1 + 3\sqrt2$, $5$) | 1 of 9 ($7$) | 8 of 22 | 12 of 36 |
| Z[√3] | 1 of 1 ($3 + 2\sqrt3 = \sqrt3\,\varepsilon$) | 0 of 0 | 1 of 5 ($3\varepsilon^2$) | 0 of 4 | 5 of 13 | 6 of 25 | 12 of 52 |
| Z[(1+√5)/2] | 1 of 1 ($\sqrt5$) | 1 of 3 ($5$) | 5 of 13 | 21 of 49 | | | |

Z[√2] at $\sqrt2^2$ has multipliers in the norm window ($|N| = 7$) and none in the per-embedding window. Z[√3] at $\pi^4$ is the same, and in Z[√3] the first modulus has the balanced $3 + 2\sqrt3$. The analogue of 3x+1 by size is $3 + 2\sqrt3$ at $\pi$ in Z[√3] (one odd class, $D = (-0.208, -0.208)$) and $\sqrt5$ at 2 in Z[(1+√5)/2] ($D = (-0.172, -0.172)$).

### 3.5 Test

`realwin.py` (log `logs/realq_window.txt`), `realwin_extra.py` (`logs/realq_window_extra.txt`). For each ring and $k$ (Z[√2] $k \le 5$, Z[√3] $k \le 4$, Z[(1+√5)/2] $k \le 3$), I took up to four classes in the norm window, the nearest class below and above it, and for each the three associates around the balanced one. Each used a smallest-digit table read literally on $R$. 60 starts had both embeddings near $2^{40}$. The slope is the mean change of $\ell_j$ per odd step while both $\ell_j > 12$ (and at most 400 steps); fates were taken after up to 3000 odd steps (escape above $2^{400}$).

| ring, $k$ | $\alpha$ | $\lvert N\rvert$, position | $D$ | measured slope | mean $v$ (pred.) | fates of 60 |
|---|---|---|---|---|---|---|
| Zr2, 2 | $1 + 2\sqrt2$ | 7, in | $(+0.437, -0.629)$ | $(+0.462, -0.604)$ | 3.006 (3) | 60 escape in $\sigma_1$ |
| Zr2, 2 | $3 - \sqrt2$ | 7, in | $(-0.835, +0.642)$ | $(-0.826, +0.651)$ | 2.998 (3) | 60 escape in $\sigma_2$ |
| Zr2, 2 | $3$ | 9, above | $(+0.085, +0.085)$ | $(+0.085, +0.085)$ | 3.003 (3) | 60 still growing at cap |
| Zr2, 3 | $3$ | 9, in | $(-0.415, -0.415)$ | $(-0.359, -0.359)$ | 3.751 (4) | 33 trivial, 27 other cycle |
| Zr2, 3 | $-1 + 2\sqrt2$ | 7, below | $(-1.129, -0.063)$ | $(-1.118, -0.052)$ | 4.020 (4) | 60 trivial |
| Zr2, 4 | $1 + 3\sqrt2$ | 17, in | $(-0.110, -0.803)$ | $(-0.105, -0.798)$ | 4.998 (5) | 36 trivial, 24 other |
| Zr2, 4 | $5 + \sqrt2$ | 23, in | $(+0.181, -0.658)$ | $(+0.211, -0.628)$ | 5.000 (5) | 60 escape in $\sigma_1$ |
| Zr2, 5 | $7 + \sqrt2$ | 47, in | $(+0.073, -0.518)$ | $(+0.098, -0.493)$ | 5.994 (6) | 60 still growing at cap |
| Zr3, 1 | $\sqrt3$ | 3, in | $(-2.107, +1.692)$ | $(-1.919, +1.634)$ | 1.995 (2) | 60 escape in $\sigma_2$ |
| Zr3, 1 | $3 + 2\sqrt3$ | 3, in | $(-0.208, -0.208)$ | $(-0.203, -0.209)$ | 2.002 (2) | 48 trivial, 12 other |
| Zr3, 1 | $12 + 7\sqrt3$ | 3, in | $(+1.692, -2.107)$ | $(+1.721, -2.116)$ | 2.002 (2) | 60 escape in $\sigma_1$ |
| Zr3, 3 | $21 + 12\sqrt3$ | 9, in | $(-0.415, -0.415)$ | $(-0.375, -0.427)$ | 3.983 (4) | 19 trivial, 41 other |
| Zr3, 3 | $31 + 18\sqrt3$ | 11, in | $(+0.158, -0.699)$ | $(+0.216, -0.717)$ | 4.002 (4) | 50 escape, 9 cap, 1 trivial |
| Zr3, 4 | $50 + 29\sqrt3$ | 23, in | $(-0.603, +0.126)$ | $(-0.452, +0.080)$ | 4.998 (5) | 42 escape in $\sigma_2$, 18 cap |
| Zg5, 1 | $\sqrt5 = -1 + 2g$ | 5, in | $(-0.172, -0.172)$ | $(-0.161, -0.161)$ | 1.307 (1.333) | 60 other cycle |
| Zg5, 1 | $2 + g = \sqrt5\,g$ | 5, in | $(+0.522, -0.867)$ | $(+0.557, -0.831)$ | 1.334 (1.333) | 60 escape in $\sigma_1$ |
| Zg5, 2 | $4 + g$ | 19, in | $(+0.157, -0.575)$ | $(+0.156, -0.577)$ | 2.335 (2.333) | 60 escape in $\sigma_1$ |
| Zg5, 2 | $5$ | 25, in | $(-0.011, -0.011)$ | $(-0.001, -0.001)$ | 2.239 (2.333) | 27 escape, 33 cap (Section 3.6) |
| Zg5, 3 | $8 + g$ | 71, in | $(-0.068, -0.449)$ | $(-0.086, -0.467)$ | 3.328 (3.333) | 30 trivial, 30 other |
| Zg5, 3 | $-1 + 8g$ | 71, in | $(+0.245, -0.762)$ | $(+0.246, -0.761)$ | 3.335 (3.333) | 60 escape in $\sigma_1$ |

Over all 144 runs, the median of $\max_j|D_j - \text{slope}_j|$ is 0.02 bits, the 90th percentile 0.14 and the maximum 0.28. The larger errors are in Z[√3], where the log ratio fluctuates and the 12-bit cut ends the phase early. Escapes occurred in the embedding with $D_j > 0$ in every run with an escape. Runs with $0 < \max D_j < 0.1$ were still growing at the 3000-step cap. In 143 of the 144 runs the fate agreed with the sign of $\max_j D_j$.

Unit multipliers (extra test, 40 starts each). In Z[√2] at $k = 1$, $\alpha = \varepsilon$ has $D = (+0.272, -2.272)$, measured $(+0.295, -2.248)$, and all starts escape in $\sigma_1$; $\alpha = \varepsilon^{-1}$ escapes in $\sigma_2$; $\alpha = 1$ ($D = (-1, -1)$) gives trivial cycles or 0. In Z[√3] at $k = 1$, $\alpha = 1$ has $D = (-2.900, +0.900)$, measured $(-2.760, +0.857)$, and all starts escape in $\sigma_2$, while $\alpha = \varepsilon$ is balanced ($D = (-1, -1)$) and bounded. In Z[(1+√5)/2] at $k = 1$, $\alpha = g^{2}$ ($D = (+0.055, -2.722)$) is still growing at the cap and $g^3$ escapes. So a multiplier of norm 1, far below the norm window, diverges when one embedding expands. Random tables with digits among the three smallest gave the same slopes as the smallest tables (to 0.07 bits; `realwin_extra.py` item 2), so the digits do not matter, as the derivation says.

### 3.6 The exception: invariant lines

For $\alpha = 5$ at modulus 4 in Z[(1+√5)/2] the smallest table has, on every class with $b \equiv 1 \pmod 4$, a digit with $g$-coefficient $-1$ ($[g] \to -g$, $[1+g] \to -1-g$, $[2+g] \to 2-g$, $[3+g] \to 1-g$). Hence $S(a + g) = (5a + d_a + (5 - 1)g)/4^v$ with the $g$-coefficient of the numerator equal to 4. So $v = 2 = k$ exactly and $S(a + g) = a' + g$: the line $b = 1$ is invariant, with $a' = (5a + d_a)/4$ (checked for $|a| \le 2000$: every step stays on the line with $v = 2$). On it $|a|$ grows by $5/4$ per step, $\log_2(5/4) = 0.322$ bits in both embeddings (measured 0.3219 over 200 steps from $10^6 + 1 + g$). Random starts drift towards the rational direction, because the rational multiplier preserves $b/a$ up to the digits, and the bounded coordinate $b$ then falls into this line. This is the invariant-line divergence of catalog Section 14 (L3, L5), not an embedding effect. The rational multipliers 3 at $\sqrt2^3$ and $\sqrt2^4$ in Z[√2] show the same mechanism in a milder form (mean $v$ 3.75 and 4.55 against 4 and 5), with bounded orbits.

### 3.7 Verdict

The per-embedding window, in the log form of Section 3.1, predicts the behaviour of maps read on $R$. Divergence happens exactly when some embedding has $D_j > 0$, it happens in that embedding, and its rate is $D_j$. The norm window is neither necessary nor sufficient on $R$. The exceptions are the known invariant-line effects of rational multipliers. On $R/U$ (the cone) the imbalance is absorbed by the representative and the norm window is the relevant one, as `rings-test.md` 9.4 found; Section 5 compares the two.

## 4. How maps act on the units

### 4.1 The transfer theorem for an infinite unit group

Let $R$ be a domain with unit group $U$ (any size), $X \subseteq R$ a set of nonzero elements with $UX = X$ and $1 \in X$, and $T$ a map that respects associates, with induced map $\bar T$ on $R/U$ (QUO Proposition 2.2, which does not use finiteness). Suppose $[1]$ is periodic for $\bar T$ with least period $p$, and put $g = T^p$. Then $g(U) \subseteq U$, since $[g(u)] = \bar T^p[1] = [1]$.

Theorem 4.1. $T$ is a Collatz map on $X$ iff $\bar T$ is a Collatz map on $X/U$ in Collatz's sense and every orbit of $g$ on $U$ is eventually periodic.

Proof. If $T$ is a Collatz map, $\bar T$ is one by the first half of QUO Theorem 3.2, which does not use finiteness. Every unit $u \in X$ reaches a periodic point $y$ whose cycle contains a unit $w$. The points of that cycle in $U$ are exactly those at positions $\equiv$ that of $w$ modulo $p$ (QUO Theorem 4.1(2)), so the $g$-orbit of $u$ meets the periodic point $w$ and is eventually periodic. Conversely, take $z \in X$ and $n$ with $T^n(z) = v \in U$. The $g$-orbit of $v$ is eventually periodic, so some $g^i(v)$ is $g$-periodic, hence $T$-periodic, and it is a unit. $\square$

For finite $U$ every orbit of a self-map of $U$ is eventually periodic, which is the pigeonhole step of QUO Theorem 3.2. With $U$ infinite that step is exactly what can fail, and Theorem 4.1 says it is the only thing that can fail.

### 4.2 Maps with $T(u) = uw$: the unit-cycle theorem

Theorem 4.2. Let $T$ commute with $\pm1$ and satisfy $T(u) = uw$ for all $u \in U$, for a fixed unit $w$ (for instance $T$ commuting with $U$, with $T(1) = w$ a unit). Then $T^n(u) = uw^n$.

1. If $w = 1$, every unit is fixed: $U$ is a union of one-unit cycles, infinitely many when $U$ is infinite.
2. If $w = -1$, the units form the 2-cycles $\{u, -u\}$, again infinitely many.
3. If $w = \pm\varepsilon^j$ with $j \ne 0$, no unit is periodic: $uw^n = u$ forces $w^n = 1$, so $n = 0$. Every unit lies on an endless walk $u, uw, uw^2, \ldots$ with $|\sigma_1|$ tending to $\infty$ or $0$. The walks are the cosets of $\langle w\rangle$, and $[U : \langle w\rangle] = 2|j|$, so there are $2|j|$ of them.

So $T$ has a unit cycle iff $w$ has finite order, i.e. $w = \pm1$, the only torsion in $U$. In general, for $T$ commuting with $U$ and $[1]$ of period $p$, $T^{p}(u) = uw$ with $w = T^p(1)$. The trivial cycles are the lifts of the cycle of $[1]$, by QUO Theorem 4.2, whose proof does not need finiteness. There are $[U : \langle w\rangle]$ of them, each of length $p\,\mathrm{ord}(w)$, when $w = \pm1$; there are none when $w$ has infinite order. With Theorem 4.1: a map commuting with $U$ is a Collatz map on $R$ iff $\bar T$ is a Collatz map on $R/U$ and $w = \pm1$. This is the precise failure of A4 for an infinite unit group.

Proof of the general statement. $T^p(u) = uT^p(1) = uw$ by commuting; induction gives $T^{pn}(u) = uw^n$. Each point of a cycle over $[1]$ that lies in $U$ is $T^{pi}$ of any other, by QUO Theorem 4.1(2), so the cycle of $1$ meets $U$ in $\{w^n\}$, finite iff $w$ is torsion. The cycles over $[1]$ are the translates $uC_1$, and $uC_1 = C_1$ iff $u \in \langle w\rangle$. $\square$

### 4.3 Rotation and the choice of lift

A Collatz-type map on the cone, $(\alpha, d)$ with $d$ read on the class of the representative $f \in E$, has the commuting lift $L(uf) = u\,(\alpha f + d(f))/\pi^{v}$. The pair $(u\alpha, ud)$ gives the same map on $E$ (Proposition 2.3(1) of `fundamental-domain.md`, which uses nothing about $U$), and its lift is $L_u(z) = L(uz) = uL(z)$. Hence $L_u^p(1) = u^pL^p(1)$.

Corollary 4.3. Let $w = L^p(1) = s\varepsilon^j$. Some rotation $(u\alpha, ud)$ lifts to a Collatz map on $R$ iff $w \in \pm U^p$, i.e. iff $p \mid j$. The rotation by $\varepsilon^{-j/p}$ does it, and $-1$ switches between one-unit and two-unit cycles when $p$ is odd.

Corollary 4.4. Every Collatz map $F$ on $R/U$ has a lift that commutes with $U$ and is a Collatz map on $R$. Choose $T(uf) = u\,\mu(f)\,L_0(f)$ for any commuting lift $L_0$ and any function $\mu : E \to U$; this commutes with $U$ and induces $F$. Its $w$ is $w_0\prod_{i<p}\mu(f_i)$ over the cycle $f_0 = 1, \ldots, f_{p-1}$ of $[1]$, so $\mu(1) = w_0^{-1}$ and $\mu = 1$ elsewhere gives $w = 1$. So the failure of the transfer theorem is a property of a particular lift, not of the map on $R/U$. For Collatz-type maps the natural lifts form the torsor $\{L_u\}$, and Corollary 4.3 says when it contains a good one.

Lifts that respect associates without commuting can move the units arbitrarily, subject only to $T^p(U) \subseteq U$. For instance $T(u) = L(u)$ off a set of units and $T(\varepsilon^n) = \varepsilon^{n+1}$ for $n \ge 0$ gives a mixture of fixed units and one endless walk. Such lifts can have exactly one trivial cycle (send every unit into the cycle of 1) or any number of trivial cycles, each with finitely many units. They are not residue-class maps (Section 4.6).

### 4.4 Computation on the cone

`realunits.py`, `logs/realq_units_cone.txt`. Collatz-type maps on $E$, classified to $|N| \le 2\cdot10^4$ by `realfd.c`. For each Collatz map on $R/U$ I computed $p$ and $w = L^p(1) = s\varepsilon^j$. On up to four maps per ring and modulus, I followed the lift from every $z = uf$ ($f \in E$, $|N(f)| \le 2000$, $u = \pm\varepsilon^m$, $m \in \{-3, 0, 3\}$, about 4000 starts): every start reaches a unit, then lies on a unit cycle when $w = \pm1$ and on a walk otherwise. For maps with $p \mid j$, the rotated map was checked the same way and every start ended on a unit cycle. No exception occurred.

| ring, $k$ | digits per class | Collatz maps on $R/U$ | $w = \pm1$ | $p \mid j$, $j \ne 0$ | $p \nmid j$ |
|---|---|---|---|---|---|
| Zr2, 2 | 12 smallest | 8 of 288 | 4 | 3 ($w = \varepsilon$) | 1 ($1 + 2\sqrt2$, $p = 4$, $w = \varepsilon^2$) |
| Zr2, 3 | 5 | 19 of 1296 | 17 | 2 ($w = \varepsilon^{-1}$) | 0 |
| Zr2, 4 | 2 (smallest + 300 sampled) | 349 of 1792 | 210 | 139 | 0 |
| Zr3, 1 | 40 | 2 of 42 | 0 | 2 ($w = \pm\varepsilon^{-1}$) | 0 |
| Zr3, 3 | 4 | 255 of 2000 | 0 | 233 | 22 (all with $p \ge 2$) |
| Zr3, 4 | 2 (sampled) | 76 of 1200 | 0 | 20 | 56 ($p = 2$, $j = -3, -5$) |
| Zg5, 1 | 8 | 13 of 640 | 8 | 5 ($w = g^{\pm1}$) | 0 |
| Zg5, 2 | 2 (sampled) | 75 of 900 | 36 | 39 | 0 |

The Z[√3] column with $w = \pm1$ is empty because $\pi^2 = 2\varepsilon$. A step with $v$ divisions multiplies the class by $\pi^{-v}$, which on $R$ is $2^{-v/2}\varepsilon^{-v/2}$ up to a bounded factor, so the lift of the cone representative carries a unit drift, and $L(1) = (\alpha + d)/\pi^v$ is $\varepsilon^{-j}$ with $j \ge 1$ in every case. The rotated multiplier that cures it is the balanced associate: $\sqrt3 \to 3 + 2\sqrt3$, $1 + 2\sqrt3 \to 8 + 5\sqrt3$ or $31 + 18\sqrt3$, $2 + 3\sqrt3 \to 50 + 29\sqrt3$.

### 4.5 The quotient form of the Collatz property

Definition (the author's). $T$ is a Collatz map in the quotient form on $X$ if every orbit from $X$ eventually enters $U$ and stays there: $T(U) \subseteq U$, with no finiteness required of the unit orbits. For finite $U$ and maps with $T(U) \subseteq U$ the two definitions agree (pigeonhole). In general they differ: the cycle definition allows trivial cycles that leave $U$, such as $1 \to 4 \to 2 \to 1$ for the unaccelerated map.

Proposition 4.5. Let $T$ respect associates. Then $T$ is a Collatz map in the quotient form on $X$ iff $\bar T$ is a Collatz map on $X/U$ and $\bar T[1] = [1]$. When $T$ commutes with $U$ this holds iff $\bar T$ is a Collatz map on $X/U$ with $p = 1$, and then the units move by $u \mapsto uw$, $w = T(1)$, as in Theorem 4.2.

Proof. $T(U) \subseteq U$ iff $T(1)$ is a unit iff $\bar T[1] = [1]$, using that $T$ respects associates. Entering $U$ is reaching $[1]$. $\square$

So, for maps respecting associates, the quotient form is Collatz's own condition on $R/U$ with the fixed point $[1]$. It is strictly weaker than the cycle form exactly by the walks of Theorem 4.2(3). The maps that satisfy it in our setting are the lifts of the Collatz-type maps with $p = 1$ (`realquot.py`, `logs/realq_quotient.txt`). On each I checked $L(\pm\varepsilon^n) = \pm\varepsilon^n w$ for $|n| \le 40$ (true in every case, as it must be) and that the ~4000 starts $uf$ all enter $U$. Examples in full (digit tables on the classes of the representative):

1. Z[√2], $\sqrt2^2$, $\alpha = 1 + 2\sqrt2$, $[1] \to 1$, $[1 + \sqrt2] \to 1 - \sqrt2$: $L(1) = \varepsilon$, two endless walks $u \mapsto u\varepsilon$ (the map of `rings-test.md` 9.4).
2. Same $\alpha$, $[1] \to -1 + 2\sqrt2$, $[1 + \sqrt2] \to -1 + \sqrt2$: $L(1) = 1$, every unit fixed. Rotating example 1 by $\varepsilon^{-1}$ ($\alpha = 3 - \sqrt2$) also gives $w = 1$.
3. Same $\alpha$, $[1] \to -3 - 2\sqrt2$, $[1 + \sqrt2] \to 1 - \sqrt2$: $L(1) = -1$, units in the 2-cycles $\{u, -u\}$.
4. Z[√2], $\sqrt2^3$, $\alpha = 3$, $[1] \to 1 - 2\sqrt2$, $[3] \to -1 + 2\sqrt2$, $[1+\sqrt2] \to 1 + \sqrt2$, $[3 + \sqrt2] \to -1 - 3\sqrt2$: $L(1) = \varepsilon^{-1}$, two walks.
5. Z[√3], $\pi$, $\alpha = \sqrt3$, digit $2 - \sqrt3$: $L(1) = \varepsilon^{-1}$, two walks. With the digit $-2 - \sqrt3$, $L(1) = -\varepsilon^{-1}$, two walks $u \mapsto -u\varepsilon^{-1}$.
6. Z[√3], $\pi^3$, $\alpha = 1 + 2\sqrt3$, $[1] \to -3$, $[3] \to -1$, $[\sqrt3] \to -2 - \sqrt3$, $[2 + \sqrt3] \to -\sqrt3$: $L(1) = \varepsilon^{-2}$, four walks.
7. Z[√3], $\pi^3$, $\alpha = 4 + \sqrt3$, $[1] \to 4 - \sqrt3$, $[3] \to \sqrt3$, $[\sqrt3] \to 1$, $[2 + \sqrt3] \to -1$: $L(1) = \varepsilon^{-3}$, six walks.
8. Z[(1+√5)/2], modulus 2, $\alpha = \sqrt5$, $[1] \to 1 + 2g$, $[g] \to g$, $[1 + g] \to 1 + g$: $L(1) = g$, two walks; with $[1] \to 3 - 2g$, $[g] \to -g$, $[1 + g] \to -1 + g$: $L(1) = 1$; with $[1] \to -1 - 2g$, $[g] \to -g$, $[1 + g] \to 1 - g$: $L(1) = -1$.
9. Z[(1+√5)/2], modulus 4, $\alpha = 4 + g$: 42 maps with $L(1) = 1$ and 18 with $L(1) = g^2$ (four walks), for instance $[1] \to 3g$, $[3] \to -4+g$, $[g] \to -1-g$, $[1+g] \to -1-2g$, $[2+g] \to -1+g$, $[3+g] \to -1$, $[1+2g] \to -2+g$, $[3+2g] \to 2-g$, $[3g] \to -3+g$, $[1+3g] \to -3$, $[2+3g] \to -3-g$, $[3+3g] \to 1-2g$.

Mixtures of finite cycles and walks are impossible for these lifts (all units move by the same $w$) and possible for non-commuting lifts that respect associates (Section 4.3). The Collatz-type maps with $p > 1$ (one at $\sqrt2^2$, 51 of 255 at $(1+\sqrt3)^3$) do not satisfy the quotient form: $L(1)$ is not a unit (for example $L(1) = -12 + 7\sqrt3$, $|N| = 3$, with $L^2(1) = \varepsilon^{-4}$), although $L^p(U) \subseteq U$.

### 4.6 Residue-class maps on the units

Proposition 4.6. A residue-class map (finitely many digits) on a real quadratic ring sends only finitely many units to units.

Proof. If $S(u) = u'$ with $u, u' \in U$, then $\alpha u + d = \pi^v u'$ for a digit $d$ and some $v \ge 0$. Put $x = \alpha u/(\pi^v u')$ and $y = d/(\pi^v u')$. Then $x + y = 1$ with $x, y$ in the finitely generated subgroup $\Gamma = \langle -1, \varepsilon, \pi, \alpha, d\rangle$ of $K^*$. The equation $x + y = 1$ has finitely many solutions in $\Gamma$ (Siegel, Mahler, Lang; the unit equation theorem). The solution determines $\pi^v u' = d/y$, hence $v$ (the $\pi$-valuation) and $u'$, and then $u = x\pi^v u'/\alpha$. There are finitely many digits. $\square$

Consequences. No Barina-type map on $R$ satisfies $S(u) = uw$ for all units, or the quotient form; the latter needs $S(U) \subseteq U$, which fails for all but finitely many units. This is a second route to "no Barina-type map is a map on $R/U$" (`rings-test.md` 9.3). Unit cycles and the units on mixed trivial cycles are finitely many in number when the orbits are bounded. Units with large $|\sigma_1|$ or $|\sigma_2|$ leave $U$, and by the per-embedding contraction their orbits fall into the bounded region. In the census, at most 36 of the 802 units $\pm\varepsilon^n$ with $|n| \le 200$ were sent to units by any table tested (`logs/realq_quotient.txt`, Part 2).

How units move in the literal Collatz maps of Section 5. For $\alpha = 1 + 3\sqrt2$ at $\sqrt2^4$ with the first table:

$\varepsilon^{-5} \to \varepsilon^{-4} \to \varepsilon^{-3} \to \varepsilon^{-2} \to \varepsilon^{-1}$ (each with $v = 6$), with $\varepsilon^{-1}, 1, \varepsilon, \varepsilon^2$ fixed ($v = 5$), $\varepsilon^{3} \to 5 + 3\sqrt2$ and $\varepsilon^{n}$ for $n \ge 3$ or $n \le -6$ leaving $U$ (`logs/realq_unitmap.txt`, Part 2). The picture is finite walks ending at fixed units, plus 5-cycles through $\pm\varepsilon^3, \pm\varepsilon^4$ that leave $U$ and return. The fixed units come from $\eta = \pi^5 - \alpha = \sqrt2 - 1 = \varepsilon^{-1}$, a unit. By the lemma of `rings-test.md` 10.4 (with the identity as fundamental domain), $z = \eta^{-1}d(c)$ is fixed whenever it lies in $c$, and it is a unit whenever the digit $d(c)$ is a unit. All eight digits of this table are units. The walk steps come from $S(u) = u\varepsilon$, i.e. $d(c) = u(\pi^6\varepsilon - \alpha)$ with $\pi^6\varepsilon - \alpha = 7 + 5\sqrt2 = \varepsilon^3$. For $-1 + 3\sqrt2$, $\pi^5 - \alpha = 1 + \sqrt2 = \varepsilon$ and the walk runs upward, $\varepsilon^4 \to \varepsilon^3 \to \varepsilon^2 \to \varepsilon \to 1$. So forced fixed points, which kill maps in finite unit groups when they are non-units (K1, K2), here produce runs of fixed units when the digits are units.

### 4.7 Rearranging trivial cycles

For commuting lifts of Collatz-type maps the shape is determined by $w$. One unit per cycle ($w = 1$) and two per cycle ($w = -1$) are exchanged by $\alpha \to -\alpha$ when $p$ is odd. Cycles and walks are exchanged by rotations when $p \mid j$ (Corollary 4.3), and by digit changes, which move $w$ freely (examples 1 to 3 of Section 4.5 differ only in digits). Longer unit cycles are impossible for commuting lifts and possible for lifts that respect associates.

For literal residue-class maps the counts are not constrained by any unit-group formula. The census of trivial cycles through $\pm\varepsilon^n$, $|n| \le 40$ (`realunitmap.py`, Part 1) gave:

| ring, $k$, $\alpha$ | tables | trivial cycles per map | units per trivial cycle |
|---|---|---|---|
| Zr2, 4, $1 + 3\sqrt2$, commuting with $-1$ | 500 | 0 to 12 (0: 2 maps, 1: 4, 4: 110, 6: 103, 12: 6) | 1 or 2; unit cycles and mixed cycles |
| Zr2, 4, $1 + 3\sqrt2$, not commuting | 597 | 2 to 11 | 1 or 2, mostly mixed |
| Zr2, 3, $3$, commuting | 36 | 1 to 10 | 1, 2, and one map with a single cycle carrying 4 units |
| Zr3, 1, $3 + 2\sqrt3$ | 60 | 0 to 5 (0: 22, 1: 22) | 1, 2 or 3 |
| Zr3, 3, $3\varepsilon^2$, commuting | 36 | 0 to 8 | 1 or 2 |
| Zg5, 1, $\sqrt5$ | 640 | 0 to 9 | 1 or 2 |
| Zg5, 2, $5$, commuting | 601 | 2 to 20, all even | 1 or 2 |
| Zg5, 2, $5$, not commuting | 596 | 5 to 17 | 1 or 2 |

Structural facts behind the table. For a map commuting with $-1$, $-C$ is a trivial cycle with $C$, so the count is $2a + b$, with $b$ the number of cycles with $C = -C$ (which then carry $u$ and $-u$). Exactly one trivial cycle needs $C = -C$: for example Z[√2], $\alpha = 3$ at $\sqrt2^3$ with $[1] \to 1 - 2\sqrt2$, $[3] \to -1 + 2\sqrt2$, $[1 + \sqrt2] \to 1 + 3\sqrt2$, $[3 + \sqrt2] \to -1 - 3\sqrt2$, whose only trivial cycle carries $\pm1$ and $\pm\varepsilon^{-1}$. Zg5 with $\alpha = 5$ gave only even counts, since every cycle there came with its negative. Zero trivial cycles is common: the orbits of all units then end in other cycles or escape. A large count arises through the mechanism of Section 4.6, a unit $\eta = \pi^v - \alpha$ together with unit digits. Many trivial cycles, one trivial cycle, unit cycles, mixed cycles and walks of finite length all occur by changing digits or the multiplier. The only constraints found are the pairing under $-1$ for maps commuting with $-1$ and the finiteness of Proposition 4.6.

### 4.8 The norm of the fundamental unit

Z[√2] and Z[(1+√5)/2] have $N(\varepsilon) = -1$, Z[√3] has $N(\varepsilon) = +1$. In the theory above nothing depends on it. Theorems 4.1 and 4.2 and Corollaries 4.3 and 4.4 use only $U = \{\pm1\} \times \langle\varepsilon\rangle$. The cone is the same, since $|\sigma_2(\varepsilon)| = \varepsilon^{-1}$ in both cases. Observed effects:

1. Along a walk $u \mapsto u\varepsilon^j$ the sign of $N$ alternates iff $N(\varepsilon) = -1$ and $j$ is odd (Z[√2] and Z[(1+√5)/2] walks with $w = \varepsilon^{\pm1}$). In Z[√3] every unit has norm $+1$, so the sign of $N$ is constant on classes of associates and $R/U$ splits by the sign of $N$.
2. Z[(1+√5)/2] behaves like Z[√2] in every count above (cycles for $w = \pm1$ in about half the cone maps, walks with $w = g^{\pm1}, g^2$ otherwise, no maps with $p \nmid j$ found at moduli 2 and 4). Z[√3] differs: no $w = \pm1$ on the cone representative, and many $p \nmid j$. The cause is the unbalanced generator ($\pi^2/2 = \varepsilon$ is not $\pm$ a square of a unit), not the norm of $\varepsilon$.
3. In Z[(1+√5)/2], $g$ has order 3 modulo 2, so every odd class modulo 2 contains units and all three digits of a modulus-2 map can be units; tables with many fixed units appear already at the first modulus (up to 9 trivial cycles for $\sqrt5$). In Z[√2], $\varepsilon \equiv 1 \pmod\pi$ and the units meet fewer odd classes at small moduli. This comes from 2 being inert in Z[(1+√5)/2] and is unrelated to $N(\varepsilon)$.

## 5. Barina-type maps commuting with $\pm1$

### 5.1 Threshold

The units $\pm1$ act freely on the odd classes modulo $\pi^k$ iff $k \ge \kappa + 1$ with $\kappa = v_\pi(-1 - 1) = v_\pi(2)$: $\kappa = 2$ in Z[√2] and Z[√3], $\kappa = 1$ in Z[(1+√5)/2]. So tables commuting with $-1$ exist from $\sqrt2^3$, $(1+\sqrt3)^3$ and $4$, and no table commutes with all of $U$ at any modulus, since some $\varepsilon^n \ne 1$ is $\equiv 1 \bmod \pi^k$ (U2). In Z[(1+√5)/2], $g^{3\cdot2^{k-1}} \equiv 1 \pmod{2^k}$.

### 5.2 Search

`realbarina.py`, logs `logs/realq_barina_<ring>_k<k>.txt`. Multipliers: every associate with $\max_j D_j < 0.25$, so all in the per-embedding window plus the nearest unbalanced ones as controls. Tables commuting with $-1$ (C), a digit per orbit $\{c, -c\}$ among the $n$ smallest, and all tables (N) with digits among the $n$ smallest, sampled where large. Filter: `realfd.c` literal mode, every odd start with $|N| \le 3000$ and then $3\cdot10^4$ in the box reaches a cycle with a unit (escape at $2^{60}$, cap 20,000 odd steps). Survivors were checked on the units $\pm\varepsilon^n$ ($|n| \le 40$) and 200 random starts near $2^{50}$.

| ring, $k$ | multipliers in the window | C tables | N tables | Collatz maps on $R$ |
|---|---|---|---|---|
| Zr2, 3 | 3 | 36 | 256 | 0 (other cycles; 3 is forced: $\pi^4 - 3 = 1$) |
| Zr2, 4 | $1 + 3\sqrt2$, $-1 + 3\sqrt2$, $5$ | 500 each | 11,664 each | 4: two C tables each for $\pm1 + 3\sqrt2$; none for 5 ($\pi^4 - 5 = -1$) |
| Zr2, 5 | 7 | 3004 (sampled) | 3016 (sampled) | 0 ($\pi^6 - 7 = 1$) |
| Zr2, 6 | eight classes | 1504 each (sampled) | 1516 each (sampled) | 0 |
| Zr3, 1 | $3 + 2\sqrt3$ | none ($k < 3$) | 60 | 0 ($\pi^2 - \alpha = 1$: every digit is a fixed point, K2) |
| Zr3, 3 | $3\varepsilon^2$ | 36 | 400 | 0 ($\pi^4 - \alpha = \varepsilon^2$) |
| Zr3, 5 | five classes | 384 each | 1504 each (sampled) | 0 |
| Zg5, 1 | $\sqrt5$ | none ($k < 2$) | 640 | 0 |
| Zg5, 2 | 5 | 972 (sampled) | 3001 (sampled) | 0 ($4 - 5 = -1$) |
| Zg5, 3 | five classes | 1502 each (sampled) | 1504 each (sampled) | 0 |

All unbalanced controls failed, mostly by escapes in the expanding embedding. The failures of multipliers in the window are short cycles through the smallest non-units (fixed points and 2- to 4-cycles of least $|N|$ 7 in Z[√2], 3 in Z[√3], 5 in Z[(1+√5)/2]). At the first moduli these are forced: every multiplier there with an associate in the window has $\pi^v - \alpha$ a unit (column notes). The window forces $\alpha$ to be close to $\pi^m$ in both embeddings, which makes $\pi^{v} - \alpha$ small in both embeddings, often a unit, so K1 and K2 bite more often than in rings with one place. For $3 + 2\sqrt3$ at $\pi$ this is the exact analogue of Z3 ($4 - 3 = 1$ forces the digit $\pm1$ for 3x+1 on Z): every digit is a fixed point, the unit digits leave other cycles, and no map passes.

### 5.3 The first Collatz maps on $R$

Z[√2], modulus $\sqrt2^4 = 4$, tables commuting with $-1$:

1. $\alpha = 1 + 3\sqrt2$, $[1] \to -1 + \sqrt2$, $[3] \to 1 - \sqrt2$, $[1 + \sqrt2] \to 1$, $[3 + \sqrt2] \to 3 - 2\sqrt2$, $[1 + 2\sqrt2] \to -1 - \sqrt2$, $[3 + 2\sqrt2] \to 1 + \sqrt2$, $[1 + 3\sqrt2] \to -3 + 2\sqrt2$, $[3 + 3\sqrt2] \to -1$. 12 trivial cycles: fixed units $\pm1$, $\pm\varepsilon$, $\pm\varepsilon^{-1}$, $\pm\varepsilon^2$, and 5-cycles through $\pm\varepsilon^3$ and $\pm\varepsilon^4$.
2. The same with $[3 + \sqrt2] \to 3 + 2\sqrt2$, $[1 + 3\sqrt2] \to -3 - 2\sqrt2$ (the other tie): 12 trivial cycles, fixed $\pm1, \pm\varepsilon, \pm\varepsilon^2, \pm\varepsilon^3$, 5-cycles through $\pm\varepsilon^4$, $\pm\varepsilon^5$.
3. $\alpha = -1 + 3\sqrt2$ (the conjugate up to sign), $[1] \to 1 + \sqrt2$, $[3] \to -1 - \sqrt2$, $[1 + \sqrt2] \to 3 - 2\sqrt2$, $[3 + \sqrt2] \to 1$, $[1 + 2\sqrt2] \to 1 - \sqrt2$, $[3 + 2\sqrt2] \to -1 + \sqrt2$, $[1 + 3\sqrt2] \to -1$, $[3 + 3\sqrt2] \to -3 + 2\sqrt2$: 10 trivial cycles, eight fixed units and two 10-cycles $C = -C$ through $\pm\varepsilon^{-4}$ and $\pm\varepsilon^{-5}$.
4. Its other tie, $[1 + \sqrt2] \to 3 + 2\sqrt2$, $[3 + 3\sqrt2] \to -3 - 2\sqrt2$: 10 trivial cycles.

Each passes on every odd start with $|N| \le 2\cdot10^5$ in the box $|a| \le 1118$, $|b| \le 805$ (at most 215 odd steps; `realconfirm.py`, `logs/realq_confirm.txt`), on the units $\pm\varepsilon^n$ for $|n| \le 40$, and on 200 random starts near $2^{50}$. Drift $D = (-0.110, -0.803)$, respectively $(-0.803, -0.110)$. No non-commuting table with digits among the three smallest passes for these multipliers (the two N passes per multiplier are the commuting tables).

### 5.4 Comparison with Collatz-type maps on $R/U$

| ring | first modulus with multipliers (norm window) | first Collatz-type map on $R/U$ (cone) | threshold for $\pm1$ | first literal Collatz map on $R$ |
|---|---|---|---|---|
| Z[√2] | $\sqrt2^2$ | $\sqrt2^2$: $1 + 2\sqrt2$, smallest digits | $\sqrt2^3$ | $\sqrt2^4$: $\pm1 + 3\sqrt2$, commuting with $-1$ |
| Z[√3] | $\pi$ | $\pi$: $\sqrt3$, digit $\pm(2 - \sqrt3)$ | $\pi^3$ | none to $\pi^5$ (sampled) |
| Z[(1+√5)/2] | 2 | 2: $\sqrt5$ (13 maps) | 4 | none to 8 (sampled) |

The two notions differ in three ways. On $R/U$ the cone absorbs the imbalance between the embeddings, every multiplier class in the norm window is usable, and the obstruction to lifting is only the unit $w$ (Section 4). On $R$ only the classes with an associate in the per-embedding window are usable (Section 3.4), those are close to $\pi^m$ and so frequently forced, and the infinite supply of units feeds forced fixed points and fixed units. The literal Collatz maps found have many trivial cycles (10 or 12), where the induced maps on $R/U$ have one class $[1]$. In Barina's setting on Z the commuting map $7x \pm 1$ is the Collatz map on $\mathbb{N} = \mathbb{Z}/\{\pm1\}$. Here the analogue of $\mathbb{N}$ for maps commuting with $\pm1$ is $R/\{\pm1\}$, which still contains the infinite group $\langle\varepsilon\rangle$, and the Collatz property on it must hold for every one of the infinitely many units separately.

## 6. Status and files

Proved here: Section 3.1 (reduction to the norm window, product of the upper ends), Proposition 3.1, the drift identity of Section 3.2 (under Haar measure), Theorem 4.1, Theorem 4.2, Corollaries 4.3 and 4.4, Proposition 4.5, Proposition 4.6 (given the unit equation theorem), the invariant line of Section 3.6 (direct computation from the table, checked for $|a| \le 2000$), and the threshold of Section 5.1. Computed: everything else. Bounds: the window runs use 60 starts per case and 3000 odd steps; the cone classifications go to $|N| \le 2\cdot10^4$; the literal searches use the box with $|N| \le 3\cdot10^4$, and the four Collatz maps on $R$ use $|N| \le 2\cdot10^5$; sampled counts are marked. Nothing here is in Lean.

Code in `collatz/research/`: `realq.py` (arithmetic, embeddings, cone, classes; correct class representatives), `realfd.c` and `realrun.py` (engine), `realwin.py`, `realwin_extra.py`, `realbal.py` (Section 3), `realunits.py`, `realquot.py`, `realunitmap.py` (Section 4), `realbarina.py`, `realconfirm.py` (Section 5). Logs in `logs/`: `realq_window.txt`, `realq_window_extra.txt`, `realq_balanced.txt`, `realq_units_cone.txt`, `realq_quotient.txt`, `realq_unitmap.txt`, `realq_barina_<ring>_k<k>.txt`, `realq_confirm.txt`.

Not done, and natural next steps: full rather than sampled searches at $\sqrt2^5$ and beyond, and at $(1+\sqrt3)^5$ and 8, for literal Collatz maps; longer verification of the four maps of Section 5.3; a census of literal Collatz maps by number of trivial cycles at $\sqrt2^4$ with more digits.
