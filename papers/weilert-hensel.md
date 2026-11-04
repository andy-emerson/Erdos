# Hensel quotients for the Gaussian gcd and the quartic residue symbol

*Draft research note, November 4, 2026. Not for circulation until the checks listed in section 8 are done.*

Two marks are used throughout: [unproven] marks a claim supported by computation only, and [unverified] marks a bibliographic or attributive claim not checked against the primary source.

## Abstract

Weilert's $(1+i)$-ary algorithm is the Gaussian counterpart of the binary gcd: it subtracts, removes factors of $1+i$, and repeats. In 2002 he remarked that a gcd of this kind does not give a quotient sequence "in an obvious manner", and for that reason built his fast quartic residue symbol on the Euclidean, top-down gcd. We describe two bottom-up quotient steps for $\mathbb{Z}[i]$. The first takes $q \equiv x/y \pmod{2^j}$ from the low bits, with $j$ set by the size difference of $x$ and $y$. The second is a Gaussian form of the Stehlé–Zimmermann binary division, in which $q$ is chosen by valuations alone. Both preserve the gcd exactly, and both carry the quartic residue symbol, because every correction to the symbol depends only on the number of factors of $1+i$ removed and on a unit, which are read from residues mod 16. We prove linear step bounds for the first step (133 steps for 31-bit inputs at the offset used in our earlier study, falling to 73) and for the second when its precision parameter $\delta$ is at least 2; termination at $\delta = 1$ is open. All four symbol algorithms considered agree with Euler's criterion on every case tested. A recursive version with an $O(M(n)\log n)$ bound, which would answer Weilert's remark directly, is not attempted here.

## 1. Introduction

The binary gcd of two odd integers replaces the larger by the odd part of their difference. It never divides, and it reads the numbers from the low end: the factors of 2 removed after each subtraction are decided by the last bits. Weilert transferred this to the Gaussian integers [W1]. In $\mathbb{Z}[i]$ the prime $\pi = 1+i$ plays the part of 2. Both values are kept prime to $\pi$ and normalised so that their difference is divisible by $\pi^3$, the larger is replaced by the normalised part of the difference, and the loop repeats. Weilert's paper also gives the matching $O(n^2)$ algorithm for the quartic residue symbol, in the way that the binary gcd gives the binary Jacobi symbol in $\mathbb{Z}$ (according to [Wik], section 1.1; [unverified] against [W1] itself).

For fast algorithms the binary loop has a defect. A subquadratic gcd works by recursion on a quotient sequence: the first half of the quotients depends only on the leading half of the input, so it can be computed from a smaller problem. Weilert's fast quartic symbol [W3] is built on the Euclidean gcd for this reason. Brent and Zimmermann quote him [BZ10, section 5]: "We are not able to use a GCD calculation in Z[i] similar to the binary GCD algorithm … because we do not get a corresponding quotient sequence in an obvious manner." [unverified: quoted second-hand.] Brent and Zimmermann then gave an $O(M(n)\log n)$ binary algorithm for the Jacobi symbol in $\mathbb{Z}$, built on the binary recursive gcd of Stehlé and Zimmermann [SZ04]. They do not return to $\mathbb{Z}[i]$.

The setting is the contrast between top-down and bottom-up division described in [MCA, section 1.4.8]. Classical division cancels the high digits of the dividend and reads the quotient from the top. Hensel division cancels the low digits: for odd $y$ it takes $q \equiv x y^{-1} \pmod{2^k}$, so that $x - qy$ is divisible by $2^k$. As [MCA] puts it, "most MSB-division variants … have their LSB-counterpart", and "LSB-division does not need any correction step, since the carries go in the direction opposite to the cancelled bits". The binary gcd is the bottom-up form of Euclid, and Stehlé and Zimmermann's binary division is the bottom-up form of a Euclidean quotient. The question here is what the bottom-up quotient looks like in $\mathbb{Z}[i]$, whether it terminates with a useful bound, and whether the quartic symbol can ride on it.

Our answers are partial. The Hensel quotient step works and halves the step count of the $(1+i)$-ary loop. The valuation-driven step, which is the one a recursive algorithm needs, has a proven linear bound for $\delta \ge 2$. The quartic symbol needs no extra care with either step, which is the main contrast with $\mathbb{Z}$, where Brent and Zimmermann had to keep quotients positive. Section 8 says what is believed new and what has still to be checked.

## 2. Background

### 2.1 The $(1+i)$-adic valuation

Write $\pi = 1+i$. Since $2 = -i\pi^2$, the ideal $(2)$ is the square of the prime ideal $(\pi)$, and $\mathbb{Z}[i]/(\pi)$ is the field with two elements. For $z \ne 0$ let $v(z)$ be the exponent of $\pi$ in $z$. Since $N(\pi) = 2$, we have $v(z) = v_2(N(z))$, so the valuation of $z$ can be read from its norm. We call $z$ **odd** when $v(z) = 0$, which for $z = c + di$ means $c + d$ odd. Removing one factor of $\pi$ from $z = c + di$ with $c + d$ even gives $z/\pi = \tfrac{c+d}{2} + \tfrac{d-c}{2}i$, so $|z/\pi| = |z|/\sqrt2$. Each factor of $\pi$ is worth half a bit of size, and $2^j$ is worth $2j$ factors.

An odd $y$ is a unit modulo every power of $\pi$, because $N(y)$ is odd and $y \bar y = N(y)$. So $y^{-1} \bmod 2^j$ is $\bar y \cdot N(y)^{-1} \bmod 2^j$, with an ordinary integer inverse.

### 2.2 Primary elements

Every nonzero integer has two associates, $n$ and $-n$, and the usual convention picks the positive one. The Jacobi symbol's reciprocity law is stated for positive odd arguments, and the sign has to be tracked whenever a negative number appears. Every nonzero Gaussian integer has four associates, $z$, $iz$, $-z$ and $-iz$. For an odd element there is a convention that does the same job as positivity, but it is a congruence instead of an inequality.

An odd $z$ is **primary** when $z \equiv 1 \pmod{\pi^3}$. Since $\pi^3 = -2 + 2i$ generates the same ideal as $2 + 2i$, for $z = c + di$ this means $d$ even and $c + d \equiv 1 \pmod 4$. Each odd $z$ has exactly one primary associate. The ring $\mathbb{Z}[i]/(\pi^3)$ has 8 elements, 4 of them odd, and the four units $1, i, -1, -i$ fall in four different classes, since their differences $1 - i$, $2$ and $1+i$ have valuation at most 2. Hence $z = i^s p$ with $p$ primary and $s \in \{0,1,2,3\}$ determined by $z \bmod \pi^3$.

The difference matters for algorithms. Positivity is a property of the leading digits, and primarity is a property of the last few. A procedure that reads numbers from the low end can make its values primary at no extra cost in information, while it cannot tell whether an integer is positive.

### 2.3 The quartic residue symbol

Let $P$ be an odd prime of $\mathbb{Z}[i]$. For $\alpha$ not divisible by $P$, the **quartic residue symbol** $\left[\frac{\alpha}{P}\right]$ is the power of $i$ congruent to $\alpha^{(N(P)-1)/4}$ modulo $P$ (Euler's criterion), and it is 0 when $P \mid \alpha$. For odd $\beta$ it is extended multiplicatively over the prime factors of $\beta$. It depends only on $\alpha \bmod \beta$ and on the ideal $(\beta)$, so $\beta$ may be replaced by its primary associate.

For primary $\alpha = a + bi$ and $\beta = c + di$, prime to each other, we use these three laws [IR, chapter 9], [Lem, chapter 6] [unverified: exact locations in the books]:
$$\left[\tfrac{i}{\beta}\right] = i^{-(c-1)/2},\qquad \left[\tfrac{1+i}{\beta}\right] = i^{(c-d-d^2-1)/4},\qquad \left[\tfrac{\alpha}{\beta}\right] = (-1)^{\frac{a-1}{2}\cdot\frac{c-1}{2}}\left[\tfrac{\beta}{\alpha}\right].$$
The reciprocity sign is usually written $(-1)^{\frac{N\alpha-1}{4}\cdot\frac{N\beta-1}{4}}$. For primary $\alpha$, $\frac{N\alpha - 1}{4}$ and $\frac{a-1}{2}$ have the same parity, which gives the form above. We checked all three laws against Euler's criterion for composite primary moduli (section 6). Write $k_i(\beta)$ and $k_1(\beta)$ for the exponents in the two supplements, taken mod 4. Both depend only on $c, d \bmod 16$, and the reciprocity sign depends only on $a, c \bmod 4$.

## 3. The Hensel-quotient step

### 3.1 The step

Let $\ell(n)$ be the bit length of a positive integer $n$. Fix an even offset $c \ge 6$.

**Step H.** Let $x, y$ be primary with $N(x) \ge N(y)$ and $y \ne 1$. Put $L = \ell(N(x)) - \ell(N(y))$ and $j = \lfloor (L + c)/2 \rfloor$. Let $q \equiv x y^{-1} \pmod{2^j}$, with both coordinates of $q$ in $[-2^{j-1}, 2^{j-1})$. If $x = qy$, stop: $y$ is the gcd. Otherwise write
$$x - qy = i^s \pi^t x', \qquad x' \text{ primary},$$
and replace the pair $(x, y)$ by $(y, x')$.

The quotient is computed as $q = (x \bar y)\cdot N(y)^{-1} \bmod 2^j$, coordinate by coordinate. The size difference $L$ costs two leading-zero counts. So the step takes one number from the top of its inputs and everything else from the bottom. For $c = 6$ this is the step of study 33 (there written $j = \max(2, (L+6) \gg 1)$, which is the same since $L \ge 0$).

### 3.2 Correctness

**Lemma 1.** In step H, $t \ge 2j$, and $\gcd(x', y) = \gcd(x, y)$ up to a unit.

*Proof.* By the choice of $q$, $x - qy \equiv 0 \pmod{2^j}$, and $v(2^j) = 2j$, so $t \ge 2j$. Any common divisor of $x$ and $y$ divides $x - qy$, and conversely any common divisor of $x - qy$ and $y$ divides $x$. So $\gcd(x - qy, y) = \gcd(x, y)$. Since $y$ is odd, $\gcd(\pi, y) = 1$, and removing $i^s\pi^t$ does not change the gcd with $y$. $\square$

Lemma 1 is where the step differs from Sorenson's $k$-ary reduction [Sor94], which finds small $u, v$ with $ux + vy \equiv 0 \pmod k$ and replaces $x$ by $(ux + vy)/k$. Multiplying $x$ by $u$ can introduce a common factor that is not in $\gcd(x,y)$, and the $k$-ary algorithm needs a final pass to remove it. Step H multiplies only $y$, and needs no such pass.

### 3.3 Size and termination

Since $|q| \le 2^{j-1}\sqrt2 = 2^j/\sqrt2$ and $|\pi^t| = 2^{t/2} \ge 2^j$,
$$|x'| \le \frac{|x| + |q|\,|y|}{2^j} \le 2^{-j}|x| + \frac{|y|}{\sqrt2}. \tag{1}$$
From the definition of $L$, $N(x) < 2^{L+1} N(y)$, so $|x| < 2^{(L+1)/2}|y|$. From the definition of $j$, $2^j \ge 2^{(L+c-1)/2}$. Hence
$$2^{-j}|x| < 2^{1 - c/2}|y|, \qquad |x'| < \left(2^{1-c/2} + 2^{-1/2}\right)|y|.$$
For $c = 6$ the factor is $0.25 + 0.707 < 0.96$. So $x'$ is always strictly smaller than $y$, the new pair $(y, x')$ is again ordered by norm, and no comparison is needed after the first step.

**Theorem 2.** Step H, repeated, ends after finitely many steps with the gcd. Let $x_k$ denote $|x'|$ after step $k$, with $x_{-1} = |x|$ and $x_0 = |y|$ at the start, and let $j_{\min} = c/2$. Then
$$x_{k+1} \le \min\!\left(2^{-j_{\min}}\,x_{k-1},\; 2^{1-c/2}\,x_k\right) + \frac{x_k}{\sqrt2}. \tag{2}$$

*Proof.* After step $k$ the pair is $(x_{k-1}, x_k)$ in magnitudes. Apply (1) to step $k+1$: the first term $2^{-j}x_{k-1}$ is at most $2^{-j_{\min}} x_{k-1}$ because $j \ge c/2$ when $L \ge 0$, and at most $2^{1-c/2}x_k$ by the estimate above. The sequence $x_k$ is strictly decreasing, and the norms are positive integers, so the loop stops, either with $x - qy = 0$ or with $y = 1$. In both cases Lemma 1 shows that the last $y$ is the gcd. $\square$

The right side of (2) is increasing in both $x_{k-1}$ and $x_k$. So iterating (2) as an equality from the largest possible start gives an upper bound on the number of steps. For inputs with coordinates below $2^{31}$ in absolute value, start from $x_{-1} = x_0 = 2^{31.5}$ and stop when the bound falls below $\sqrt5$, since a primary element of norm less than 5 is 1. The results are in table 1. The iteration was done in floating point; redoing it in interval or exact arithmetic would make the bounds fully rigorous, and has not been done.

**Table 1.** Step H for inputs with 31-bit coordinates.

| offset $c$ | proved bound on steps | observed mean / max steps, 3,000 random pairs |
|---|---|---|
| 6 | 133 | 15.17 / 23 |
| 8 | 88 | 14.90 / 22 |
| 10 | 73 | 14.88 / 22 |
| 12 | 67 | 14.80 / 22 |

Study 33 proved descent for $c = 6$ with a one-term argument: the product of the two norms falls by a factor of at least 0.916 per step, which gives about 960 steps. The two changes that bring this to 133 are to use the smaller element as the potential, and to keep both terms of (2) instead of bounding the first by the second.

The asymptotic rate of (2) is the positive root $\rho$ of $\rho^2 = \rho/\sqrt2 + 2^{-c/2}$, since in the long run $x_{k-1} \approx x_k/\rho < 2x_k$ and the first branch of the minimum is the smaller. This gives about 4.4, 2.9 and 2.4 steps per bit of coordinate at $c = 6$, 8 and 10. As $c \to \infty$, $\rho \to 1/\sqrt2$, which is 2 steps per bit of coordinate, or one bit of norm per step. That is the naive bound for Euclid with nearest rounding. A better worst case would need an argument that consecutive centred quotients cannot all lie at the corner of their square, which we have not attempted. The observed maxima are 3 to 6 times below the proved bounds.

## 4. A valuation-driven step

### 4.1 The step

Step H reads $L$ from the top, so its quotients are not a function of the low digits of the input. That is enough for a fast quadratic loop, but it cannot drive a recursive algorithm. Stehlé and Zimmermann's binary division [SZ04] uses valuations only. For integers $a, b$ with $v_2(a) < v_2(b)$ it takes $r = a + q\,b/2^{j}$, where $j = v_2(b) - v_2(a)$ and $q$ is odd with $|q| < 2^j$, chosen so that $v_2(r) > v_2(b)$. The Gaussian form follows.

**Step S.** Fix an integer $\delta \ge 1$. The state is a pair of primary elements $A, B$ and a gap $g \ge 1$, standing for the pair $(A, \pi^g B)$; at the start $g = 1$. If $B = 1$, stop. Otherwise let $q \equiv A B^{-1} \pmod{\pi^{g+\delta}}$ be the representative of smallest absolute value, found by rounding to the nearest point of the square lattice $\pi^{g+\delta}\mathbb{Z}[i]$. If $A = qB$, stop: $B$ is the gcd. Otherwise write $A - qB = i^s \pi^t C$ with $C$ primary, and replace $(A, B, g)$ by $(B, C, t - g)$.

Since $A - qB \equiv 0 \pmod{\pi^{g+\delta}}$, we have $t \ge g + \delta$, so the new gap is at least $\delta$. Correctness is Lemma 1 again: $\gcd(C, B) = \gcd(A, B)$ up to a unit. No size is compared anywhere. With $\delta = 1$ this is the exact counterpart of Stehlé and Zimmermann's step.

### 4.2 Size

In unnormalised form the step is $r_{k+1} = r_{k-1} - q_k\, r_k/\pi^{g_k}$ with $v(r_{k+1}) \ge v(r_k) + \delta$. Let $\rho(m)$ be the largest absolute value of the minimal representative of an odd class mod $\pi^m$. We computed $\rho(m)$ exactly for $m \le 11$, and for larger $m$ used the covering radius of $\pi^m\mathbb{Z}[i]$, which is $2^{(m-1)/2}$. Put $x_k = |r_k| / 2^{v(r_k)/2}$, which is the absolute value of the current primary element. Then
$$x_{k+1} \le 2^{-g_{k+1}/2}\left(2^{-g_k/2}\,x_{k-1} + \frac{\rho(g_k + \delta)}{2^{g_k/2}}\,x_k\right), \qquad g_{k+1} \ge \delta. \tag{3}$$
The gaps are not under our control, so a bound must hold for every gap sequence. We bound this by a dynamic programme over the current gap: for each gap value it keeps the componentwise maximum of $(x_{k-1}, x_k)$ over all histories, which is valid because the maps in (3) are increasing in both arguments. Table 2 gives the resulting growth factor per step and the step bound for 31-bit inputs, again stopping below $\sqrt5$.

**Table 2.** Step S for inputs with 31-bit coordinates.

| $\delta$ | growth factor per step | bound on steps | observed mean / max, 3,000 random pairs |
|---|---|---|---|
| 1 | 1.000 | none | 27.52 / 41 |
| 2 | 0.852 | 131 | 22.54 / 34 |
| 3 | 0.784 | 87 | 18.16 / 26 |
| 4 | 0.748 | 73 | 16.17 / 23 |

The bounds for $\delta \ge 2$ are computer-assisted. Two points need writing out before they count as proofs. The programme considers gaps up to 40 (60 for the step counts); larger gaps should only help, since they add a factor $2^{-g/2}$, but this has not been written as an argument [unproven]. The values $\rho(m)$ for $m \le 11$ come from an exhaustive search over a box that contains every minimal representative; that search should be checked independently.

The rates for $\delta = 2, 3, 4$ (about 4.3, 2.9 and 2.4 steps per bit of coordinate) are close to those of step H at $c = 6, 8, 10$. At $\delta = 4$ step S has the same bound as step H at $c = 10$ and a mean within 10% of it.

### 4.3 Termination at $\delta = 1$

At $\delta = 1$ the growth factor is exactly 1, reached on runs of gap 1. On such a run $g_k = 1$, the quotient is a unit ($\rho(2) = 1$, since the odd classes mod 2 are those of $1$ and $i$), and (3) reads
$$x_{k+1} \le \tfrac12\left(x_{k-1} + x_k\right).$$
This does not force a decrease. In $\mathbb{Z}$ the same recurrence at gap 1 is $x_{k+1} \le \tfrac14(x_{k-1} + x_k)$, whose rate $\frac{1+\sqrt{17}}{8} = \left(\frac{\sqrt{17}-1}{2}\right)^{-1}$ is Stehlé and Zimmermann's constant. The difference is that a gap of one factor of 2 halves an integer, while a gap of one factor of $\pi$ divides a Gaussian integer by only $\sqrt2$. This is the Gaussian form of the weak triangle inequality that Wikström identifies as the obstacle to a direct transfer of binary methods [Wik]. One more digit of quotient precision ($\delta = 2$) recovers the margin.

Whether step S terminates for $\delta = 1$ is open. It terminated on every pair with coordinates of absolute value at most 10 (96,800 pairs, at most 6 steps) and on every random case we ran [unproven].

### 4.4 Dependence on low digits

For a recursive algorithm the quotients must depend only on low digits of the input, in the sense of [SZ04, lemma on truncation]: the quotients that bring the valuation up to $v$ are determined by the inputs mod $\pi^{2v + O(1)}$. We tested this by perturbation. For random 40-bit primary $A, B$ and random $M$ between 10 and 60, we replaced $A$ and $B$ by $A + \pi^M X$ and $B + \pi^M Y$ with small random $X, Y$, ran both sequences, and found the first step whose $(q, t, s)$ differed. In 1,500 trials for each of $\delta = 1, 2, 4$, that step always came when $2v(B) \ge M - 15$ (the worst cases were $M - 10$, $M - 11$ and $M - 15$). This is consistent with the truncation property, but it is a test and not a proof [unproven].

## 5. The symbol rides on the loop

**Lemma 3.** Let $y$ be primary, $y \ne 1$, let $x, q \in \mathbb{Z}[i]$, and suppose $x - qy = i^s\pi^t x' \ne 0$ with $x'$ primary. Then
$$\left[\tfrac{x}{y}\right] = i^{\,t\,k_1(y) + s\,k_i(y)}\left[\tfrac{x'}{y}\right].$$

*Proof.* The symbol depends only on the numerator mod $y$, and $x \equiv x - qy \pmod y$. It is multiplicative in the numerator, so $\left[\frac{x - qy}{y}\right] = \left[\frac{i}{y}\right]^s\left[\frac{1+i}{y}\right]^t\left[\frac{x'}{y}\right]$. $\square$

The quotient $q$ does not appear in the correction. So the rule that chooses $q$ is free, and Lemma 3 applies equally to Weilert's step ($q$ a unit), to step H, to step S and to Euclid with nearest rounding. Each loop computes $\left[\frac{a}{b}\right]$ as follows. Make $b$ primary, remove the powers of $\pi$ and the unit from $a$ by Lemma 3, and keep an exponent $e \bmod 4$. At each step add $t\,k_1(y) + s\,k_i(y)$, and whenever the roles of the two values swap, add 2 if $x$ and $y$ both have real part $\equiv 3 \pmod 4$ (reciprocity). The loop ends either with $y = 1$, and then the symbol is $i^e$, or with $x - qy = 0$ and $y \ne 1$, and then $\gcd(a, b) \ne 1$ and the symbol is 0.

The extra work per step is small. The numbers $t$ and $s$ are already known from removing $\pi^t$ and making the result primary. The correction needs $t \bmod 4$, a table of 256 entries or a few operations on $(c \bmod 16, d \bmod 16)$, and one AND of bit 1 of two real parts for the reciprocity sign. We estimate about 10 operations per step; this was not counted in an operation model [unverified estimate].

In $\mathbb{Z}$ the binary Jacobi symbol is delicate for an archimedean reason. Reciprocity is stated for positive arguments, so Brent and Zimmermann had to arrange positive quotients, and they note that the centred binary quotient "does not appear to give a subquadratic algorithm" [BZ10]. In $\mathbb{Z}[i]$ the role of positivity is taken by primarity (section 2.2), which is a congruence mod $\pi^3$, and the corrections of Lemma 3 and the reciprocity sign depend only on residues mod 16. So the running exponent $e$ is a function of the low digits of the quotient sequence alone. In particular, step S, whose quotients depend only on low digits (section 4.4, tested), carries the symbol with no sign problem. That is the quotient sequence Weilert's remark says was missing; what is still missing is a proof that it supports a recursion (section 8.3).

## 6. Numerical verification

We implemented four symbol algorithms in Python with exact integer arithmetic:

- W: Weilert's step, in the form of Damgård and Frandsen [DF]: both values primary, the larger replaced by the primary part of the difference;
- H: step H with $c = 6$;
- E: Euclid with nearest rounding and the remainder made primary, as in Eisenstein's scheme presented by Bach and Sandlund [BS];
- S: step S with $\delta = 1$ unless stated.

The reference is Euler's criterion applied to each prime factor of the modulus, with the factorisation known by construction. The checks were:

1. The supplements and reciprocity of section 2.3, on 3,000 random composite primary moduli with one to four prime factors (both split and inert primes) for the supplements, and on 2,780 coprime pairs of such products for reciprocity. No failures.
2. All four algorithms, exhaustively, over the 234 primary moduli of norm at most 600, each in all four associates, against every numerator $a$ with $|\mathrm{Re}\,a|, |\mathrm{Im}\,a| \le 8$, $a \ne 0$: 269,568 cases. No mismatches.
3. W, H and E on 4,000 random cases, and all four on a further 3,000, with composite moduli of up to four prime factors of up to 30 bits each and 40-bit numerators, including numerators divisible by $1+i$ and numerators sharing a factor with the modulus. No mismatches.
4. Mutual agreement of all four, and of H at $c = 6, 8, 10, 12$ and S at $\delta = 1, 2, 3, 4$, on 3,000 random pairs with 31-bit coordinates, of which 2,639 have a nonzero symbol. No disagreements.

The step counts of the symbol algorithms are those of the underlying gcd loops, since the symbol adds no steps.

**Table 3.** Steps per symbol (equivalently per gcd), 3,000 random pairs with 31-bit coordinates.

| algorithm | mean | max | proved bound |
|---|---|---|---|
| W ($\varepsilon$ step) | 29.43 | 39 | about 126, from $\log_2 N(xy)$ [DF] |
| H, $c = 6$ | 15.17 | 23 | 133 |
| H, $c = 10$ | 14.88 | 22 | 73 |
| S, $\delta = 1$ | 27.52 | 41 | none |
| S, $\delta = 2$ | 22.54 | 34 | 131 |
| S, $\delta = 4$ | 16.17 | 23 | 73 |
| E (Euclid, nearest rounding) | 14.71 | 22 | 63 (naive) |

The scripts are in `research/weilert-adic/`; `t1.py`, `t7.py`, `t2.py`, `t3.py` and `t5.py` are checks 1 to 4 and table 3, `t5.py` and `t6.py` the bounds, `t4.py` the exhaustive termination test of section 4.3, and `t8.py` the perturbation test of section 4.4. All were rerun for this draft. Nothing was timed. Steps H and S take about half the steps of W, at the cost of an inverse mod $2^j$ per step; study 33 estimated the trade at a 36% saving in its operation model for 31-bit inputs.

## 7. Related work

The binary gcd is Stein's; its Gaussian form is Weilert's [W1], who also gave a Schönhage-type fast gcd for $\mathbb{Z}[i]$ [W2] and the $n^{1+o(1)}$ quartic symbol based on it [W3]. Wikström credits [W2] with a Gaussian form of Sorenson's $k$-ary reduction [Wik] [unverified against W2]. Damgård and Frandsen [DF] gave binary-style gcd and cubic residue algorithms for the Eisenstein integers and sketched the transfer to $\mathbb{Z}[i]$; their bound is that the product of the norms at least halves per step, and they do not cite Weilert. Agarwal and Frandsen extended binary-like gcds to other quadratic rings [AF04, AF06] [unverified: not read]. Wikström studied $\ell$-ary gcds in rings of integers and named the weak triangle inequality as the obstacle [Wik]. Bach and Sandlund [BS] showed that a Gaussian Euclid with exact rounded quotients costs $\Omega(n M(n))$ in the worst case and that approximate quotients bring it to $O(n^2)$; they also state the quartic laws used here. In $\mathbb{Z}$, Sorenson's $k$-ary reduction [Sor94] and the related accelerated gcd of Jebelean and Weber are two-sided relatives of step H. Stehlé and Zimmermann's binary recursive gcd [SZ04] is the prototype of step S, and Brent and Zimmermann's binary Jacobi symbol [BZ10] is the prototype of the algorithm sketched in section 8.3. The top-down and bottom-up division framework is from [MCA, section 1.4.8].

## 8. Discussion

### 8.1 What is believed new

As far as the readable literature shows, the following are new. The one-sided Hensel-quotient step for $\mathbb{Z}[i]$ with a size-guided precision, with Lemma 1 and the bounds of table 1. The valuation-driven Gaussian Stehlé–Zimmermann step, the observation that $\delta = 1$ sits exactly at the weak-triangle threshold, and the bounds for $\delta \ge 2$ in table 2. The observation that in $\mathbb{Z}[i]$, unlike $\mathbb{Z}$, all symbol corrections are 2-adic, so that the quartic symbol can follow a bottom-up quotient sequence with no sign problem.

What is not new: the subtract-and-strip gcd and quartic symbol in $O(n^2)$ [W1, DF]; the quartic laws; the Euclidean $n^{1+o(1)}$ symbol [W3]; the integer prototypes of each step [Sor94, SZ04]. Lemma 3 is elementary and is implicit in Eisenstein's scheme as presented in [BS].

### 8.2 What must be checked before any claim of novelty

None of Weilert's papers has been read; every statement about them in this note is from abstracts or from citations in [Wik], [BZ10], [BS] and a mailing-list post. The author should obtain:

1. A. Weilert, "(1+i)-ary GCD computation in $\mathbb{Z}[i]$ as an analogue to the binary GCD algorithm", J. Symbolic Comput. 30(5) (2000) 605–617. To check: the exact step (unit chosen mod $\pi^3$, or best of four), the step constant, the quartic symbol algorithm, and whether a Hensel or one-sided quotient appears.
2. A. Weilert, "Asymptotically fast GCD computation in $\mathbb{Z}[i]$", ANTS-IV, LNCS 1838 (2000) 595–613. To check: the Gaussian $k$-ary reduction credited to it by Wikström, and whether it contains a one-sided form equivalent to step H.
3. A. Weilert, "Fast computation of the biquadratic residue symbol", J. Number Theory 96 (2002) 133–151. To check: the exact wording and context of the remark quoted in section 1, and whether a binary approach is discussed further.
4. Weilert's doctoral thesis (Bonn, about 2000), which may contain fuller versions of all three [unverified: title, year and existence not checked].
5. S. Agarwal and G. S. Frandsen, "Binary GCD like algorithms for some complex quadratic rings", ANTS-VI, LNCS 3076 (2004), and "A new GCD algorithm for quadratic number rings with unique factorization", LATIN 2006, LNCS 3887 [unverified: details from memory and metadata only]. To check: whether either contains a Hensel-type or valuation-driven quotient for $\mathbb{Z}[i]$.
6. D. Wikström, "On the l-ary GCD-algorithm in rings of integers", ICALP 2005 [unverified: details from memory; only the KTH report [Wik] was read].

If any of these contains step H or step S, the note reduces to the improved bounds and the observation of section 5.

### 8.3 What would make a full paper

An exposition of steps H and S with their bounds is probably too slight for a journal. The result that would justify a paper is a binary recursive quartic symbol: step S with $\delta \ge 2$, a proved truncation lemma replacing the test of section 4.4, a half-gcd recursion on $2 \times 2$ matrices over $\mathbb{Z}[i]$ with the symbol exponent carried alongside, and an $O(M(n)\log n)$ bound. That would answer Weilert's remark in the way [BZ10] answered the corresponding question in $\mathbb{Z}$, and it should be simpler than his top-down algorithm. None of the recursion has been written or proved here. The other open points are termination of step S at $\delta = 1$, and worst-case bounds closer to the observed maxima, which are 3 to 6 times smaller than the proved ones.

## References

[AF04] S. Agarwal, G. S. Frandsen, Binary GCD like algorithms for some complex quadratic rings, ANTS-VI, LNCS 3076 (2004). [unverified]

[AF06] S. Agarwal, G. S. Frandsen, A new GCD algorithm for quadratic number rings with unique factorization, LATIN 2006, LNCS 3887. [unverified]

[BS] E. Bach, B. Sandlund, On Euclidean methods for cubic and quartic Jacobi symbols, ANTS-XIII poster, 2018, arXiv:1807.07719.

[BZ10] R. P. Brent, P. Zimmermann, An $O(M(n)\log n)$ algorithm for the Jacobi symbol, ANTS IX, LNCS 6197 (2010), arXiv:1004.2091. [LNCS volume from memory, unverified]

[DF] I. B. Damgård, G. S. Frandsen, Efficient algorithms for gcd and cubic residuosity in the ring of Eisenstein integers, BRICS report RS-03-8 (2003).

[IR] K. Ireland, M. Rosen, A Classical Introduction to Modern Number Theory, 2nd ed., Springer, 1990, chapter 9. [unverified: chapter not rechecked]

[Lem] F. Lemmermeyer, Reciprocity Laws: From Euler to Eisenstein, Springer, 2000. [unverified: chapter not rechecked]

[MCA] R. P. Brent, P. Zimmermann, Modern Computer Arithmetic, Cambridge University Press, 2010, section 1.4.8.

[Sor94] J. Sorenson, Two fast GCD algorithms, J. Algorithms 16 (1994) 110–144.

[SZ04] D. Stehlé, P. Zimmermann, A binary recursive gcd algorithm, ANTS-VI, LNCS 3076 (2004) 411–425. [page numbers from memory, unverified]

[W1] A. Weilert, (1+i)-ary GCD computation in $\mathbb{Z}[i]$ as an analogue to the binary GCD algorithm, J. Symbolic Comput. 30(5) (2000) 605–617. [not read]

[W2] A. Weilert, Asymptotically fast GCD computation in $\mathbb{Z}[i]$, ANTS-IV, LNCS 1838 (2000) 595–613. [not read]

[W3] A. Weilert, Fast computation of the biquadratic residue symbol, J. Number Theory 96 (2002) 133–151. [not read]

[Wik] D. Wikström, technical report on the $\ell$-ary gcd in rings of integers, KTH, 2004, https://www.csc.kth.se/~dog/research/papers/Wik04TR.pdf, section 1.1. [read in an earlier pass; exact title not recorded]
