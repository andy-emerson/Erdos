# A recursive binary gcd and quartic residue symbol in Z[i] (Nov 4, 2026)

This note continues `papers/weilert-hensel.md` (the draft) and `research/weilert-adic.md`. It takes step S of the draft (section 4), proves a truncation lemma for it, builds a half-gcd recursion on $2\times2$ matrices over $\mathbb{Z}[i]$ that carries the quartic symbol, tests it, and gives a cost argument. Notation is that of the draft: $\pi = 1+i$, $v$ the $\pi$-adic valuation, an odd $z$ is primary when $z \equiv 1 \pmod{\pi^3}$, and $k_1$, $k_i$ are the exponents of the two supplements. Three labels are used: proven (a proof is written here), tested (supported by the computations listed), and unproven. The draft and the earlier scripts were not changed. Code: `rec.py` (the algorithms), `t9.py` (truncation test), `t10.py` (correctness), `t11.py` (operation counts), `t12.py` (contraction constants), all in `research/weilert-adic/`. Nothing was timed.

## 1. Summary

1.1 Truncation lemma: proven, for every $\delta \ge 1$. The quotients and gaps of all steps whose consumed remainder has valuation $v$ with $2v + \delta \le Q$ depend only on the inputs mod $\pi^Q$. The units also depend only on the inputs mod $\pi^Q$ when $2v + \max(\delta, 2) \le Q$, and the symbol increments when $2v + \max(\delta, 7) \le Q$. The constant $\delta$ for the quotients is sharp in tests; for the symbol data the tests show that 6 or 7 is needed.

1.2 Recursion: written and tested. It gives exactly the quotient sequence of the step-by-step loop, the gcd, and the quartic symbol, and it agreed with Euler's criterion on every case tried (moduli up to norm about $2^{2000}$, numerators up to 4096 bits).

1.3 Cost: $O(M(n)\log n)$ for $\delta \ge 2$ is proven, with a size lemma that uses only the covering radius of $\pi^m\mathbb{Z}[i]$. The constants the proof gives are very poor: the size bound shrinks by a factor $\theta = 0.9828$ per unit of valuation at $\delta = 2$. With the exact minimal-representative radii (computer-assisted) $\theta$ improves to 0.9232. The operation counts behave like $M(n)\log n$ from 128 to 8192 bits. For $\delta = 1$ the recursion works in every test and is the cheapest in the counts, but termination, and hence the cost bound, is open as in the draft.

1.4 Literature: the fast Gaussian gcd as such is not new. Weilert gave an $O(\mu(n)\log n)$ algorithm for ideal sums in quadratic orders, and so for gcds when the class number is 1 (Math. Comp. 2006). What appears new is the bottom-up (binary) recursion with the quartic symbol carried along, which is the gap named in Weilert's 2002 remark. This rests on abstracts only (section 2.4).

## 2. Literature

2.1 Stehlé and Zimmermann, "A binary recursive gcd algorithm", ANTS-VI (2004), read as a summary through WebFetch of `perso.ens-lyon.fr/damien.stehle/downloads/recbinary.pdf`. Direct download was refused by the proxy (403), so what follows is from model summaries of the PDF, not the text itself.

- GB division: for $\nu(a) < \nu(b)$ there is a unique $(q, r)$ with $r = a + q\,b/2^{\nu(b)-\nu(a)}$, $q$ odd, $|q| < 2^{\nu(b)-\nu(a)}$ and $\nu(r) > \nu(b)$.
- Lemma 6 (one step): if $a' \equiv a$, $b' \equiv b \pmod{2^l}$ with $l \ge 2\nu(b)+1$ and $0 = \nu(a) < \nu(b)$, then the quotients agree and $r' \equiv r \pmod{2^{l-\nu(b)}}$. Lemma 7: inputs mod $2^{2k+1}$ fix all quotients up to the first remainder of valuation greater than $k$.
- Half-GB-gcd: $k = \lfloor \ell(a)/2 \rfloor$. The first call uses $k_1 = \lfloor k/2\rfloor$ on the low $2k_1+1$ bits. The returned matrix is applied to the full inputs, one explicit GB division follows, and the second call uses $k_2 = k - (j_0 + j_1)$ on the low $2k_2+1$ bits. The result is $R = R_2\,[q]_{j_0}\,R_1$ and $(c, d) = 2^{-2j}R(a, b)$.
- Cost: $H(n) = 2H(n/2 + 1) + O(M(n))$ and $G(n) = H(n) + G(\lceil 0.679\,n\rceil) + O(n)$, so $O(M(n)\log n)$. The constant $0.679 = \frac12\log_2\frac{1+\sqrt{17}}{2}$ comes from the entry bound on the quotient matrices, which grows like $\left(\frac{1+\sqrt{17}}{4}\right)^d$ in the valuation $d$. The length bound is $n/\log_2\frac{\sqrt{17}-1}{2} \approx 1.555\,n$.

2.2 Brent and Zimmermann, "An $O(M(n)\log n)$ algorithm for the Jacobi symbol", ANTS IX (2010), arXiv:1004.2091, read as a summary through WebFetch of the arXiv PDF.

- They use positive quotients, $q \in [1, 2^{j+1})$ odd (BinaryDividePos), so that all values stay positive and the Jacobi symbol stays defined. Their remark: "Binary division with a centered quotient does not seem to give a subquadratic algorithm."
- Each recursive call returns $(s, j, R)$, with $s$ a single sign bit. Bits are combined as $s = s_0 + s_1 + s_2 \bmod 2$, and every correction depends on $j_0 \bmod 2$ and residues mod 8 or mod 4.
- They truncate mod $2^{2k_1+2}$ instead of $2^{2k_1+1}$, so the mod 8 corrections are right; this is the counterpart of our $c_0 = 7$.
- Positive quotients can loop without progress. The case $(j_0, q) = (1, 3)$, a "harmless iteration", is handled by a closed-form matrix power. This is their counterpart of our open $\delta = 1$ termination question.
- The quote from Weilert in their section 5 is confirmed: "We are not able to use a GCD calculation in Z[i] similar to the binary GCD algorithm … because we do not get a corresponding quotient sequence in an obvious manner."
- The cost recurrence is not spelled out in the part read; they cite SZ04.

2.3 Gaussian and other rings.

- Wikström, "On the l-ary GCD-algorithm in rings of integers", ICALP 2005, LNCS 3580, 1189–1201: read (kth.se copy). It is quadratic only and mentions no recursive or subquadratic binary method. It credits Weilert [ANTS-IV] with a Gaussian form of Sorenson's $l$-ary algorithm and Weilert [JSC] with the binary one. It names the failure of a norm triangle inequality as the obstacle and works around it with "$\Delta$-balanced" elements. It also lists the quartic symbol (Weilert) and the cubic and quartic symbols (Damgård–Frandsen, Wikström) as binary-style residue symbol algorithms.
- Agarwal and Frandsen, ANTS-VI (2004): abstract only. They give binary-like $O(n^2)$ gcds for $d = -2, -7, -11, -19$, nothing recursive.
- Weilert, "Two efficient algorithms for the computation of ideal sums in quadratic orders", Math. Comp. 75 (2006) 941–981: abstract only (AMS page). It computes ideal sums, and hence gcds in class number 1, in $O(\mu(n)\log n)$. So a fast gcd in $\mathbb{Z}[i]$ with this bound is known. From the context (his 2002 remark) it is presumably top-down, but that is not checked.
- Lichtblau, "Half-GCD and fast rational recovery" (Wolfram library): integer HGCD only, no Gaussian case, cites SZ04.

2.4 Not read: Weilert's JSC 2000, ANTS-IV 2000, JNT 2002 and Math. Comp. 2006 texts, his thesis, the Agarwal–Frandsen texts, MCA section 1.4.8. Before any claim of novelty, Math. Comp. 2006 and JNT 2002 must be read, to see whether either has a bottom-up variant or a symbol in $O(M(n)\log n)$.

## 3. Step S in state form

3.1 The draft writes step S on a triple $(A, B, g)$. For the recursion it is convenient to keep the second element unnormalised. A state is a pair $(A, Y)$ with $A$ primary and either $Y = 0$ or $v(Y) = g \ge 1$. The target symbol of the state is $\left[\frac{Y}{A}\right]$. One step from $(A, Y)$, $Y \neq 0$:

1. Write $Y = i^s\pi^g C$ with $C$ primary and add $g\,k_1(A) + s\,k_i(A) + \epsilon(C, A)$ to the exponent $e$, where $\epsilon(C,A) \in \{0, 2\}$ is the reciprocity sign. The target is now $\left[\frac{A}{C}\right]$.
2. Let $q$ be the minimal representative of $A C^{-1} \bmod \pi^{g+\delta}$ (rounding in the lattice $\pi^{g+\delta}\mathbb{Z}[i]$, exactly as in the draft's code). Put $r = A - qC$ and $Y' = r/\pi^g$, and add $g\,k_1(C)$ to $e$, since $\left[\frac{A}{C}\right] = \left[\frac{r}{C}\right] = i^{g k_1(C)}\left[\frac{Y'}{C}\right]$. The new state is $(C, Y')$.

Since $v(r) \ge g + \delta$, the new gap is $v(Y') \ge \delta$. This is the draft's step S with the normalisation of the remainder moved to the start of the next step; the quotients, gaps and units are the same (tested, section 8.1). To compute $\left[\frac{a}{b}\right]$, make $b$ primary ($B_0$), write $a = i^s\pi^t A_0$, and start from $(A_0, \pi B_0)$ with $e = t\,k_1(B_0) + s\,k_i(B_0) + \epsilon(A_0, B_0) - k_1(A_0)$. The loop ends at $Y = 0$ in a state $(G, 0)$. Then $G$ is the primary gcd, and the symbol is $i^e$ if $G = 1$ and $0$ otherwise. Every symbol identity used holds also for the unit modulus 1 ($k_1(1) = k_i(1) = 0$ and $[\cdot/1] = 1$), so the loop may run past $C = 1$. A test $A = 1$ at full precision may be used to stop early.

3.2 Matrix form. Since $\pi^2 = 2i$, the step reads
$$2^g\begin{pmatrix} C\\ Y'\end{pmatrix} = S\begin{pmatrix}A\\ Y\end{pmatrix},\qquad S = i^{-g}\begin{pmatrix}0 & i^{-s}\pi^g\\ \pi^g & -q\,i^{-s}\end{pmatrix}.$$
A sequence of steps gives $2^j (A_m, Y_m)^T = R\,(A_0, Y_0)^T$ with $R$ the product of the step matrices over $\mathbb{Z}[i]$, and $j$ the total valuation consumed, $j = v_m$, where $v_k$ is the valuation of the $k$-th remainder relative to $A_0$. Here $\pi^{2v}$ is absorbed as $2^v$ times a unit, so the Gaussian matrices carry the scaling $2^{-j}$ where SZ04 carries $2^{-2j}$. All entries of $S$ except $q\,i^{-s}$ are a unit times a power of $\pi$, so multiplying by $S$ costs one real multiplication per column.

## 4. The truncation lemma

4.1 Statement (proven). Fix $\delta \ge 1$. Let $(A_0, Y_0)$ and $(A_0', Y_0')$ be states with $A_0 \equiv A_0'$ and $Y_0 \equiv Y_0' \pmod{\pi^Q}$. Write $v_0 = 0$, $v_1 = v(Y_0)$, $v_2, \dots$ for the absolute valuations of the remainder sequence of the first pair, so step $m$ consumes the remainder of valuation $v_{m+1}$, with gap $g_m = v_{m+1} - v_m$. If $2v_{m+1} + \delta \le Q$, then steps $0, \dots, m$ of the two runs have the same gaps and quotients. If $2v_{m+1} + \max(\delta,2) \le Q$, they also have the same units $s$, hence the same step matrices. If $2v_{m+1} + \max(\delta, 7) \le Q$, they also have the same symbol increments. In each case $A_{m+1} \equiv A'_{m+1}$ and $Y_{m+1} \equiv Y'_{m+1} \pmod{\pi^{Q - 2v_{m+1}}}$. Finally, for the first $m$ with $v_{m+1} > K$, where $2K + \max(\delta,2) \le Q$, the second run also has $v'_{m+1} > K$. So "all steps with pending valuation at most $K$" is the same list for both runs. A pair with $Y = 0$ counts as having valuation $\infty$.

4.2 Proof. Induction on $m$ with the hypothesis that both runs agree up to step $m-1$, and $A_m \equiv A'_m$, $Y_m \equiv Y'_m \pmod{\pi^{P_m}}$, with $P_m = Q - 2v_m$. For $m = 0$ this is the assumption. At step $m$, $v(Y_m) = g_m$, and $g_m < P_m$ because $v_m + v_{m+1} < 2v_{m+1} + \delta \le Q$; so $v(Y'_m) = g_m$ too. Dividing by $\pi^{g_m}$, the odd parts $Y_m/\pi^{g_m}$ and $Y'_m/\pi^{g_m}$ agree mod $\pi^{P_m - g_m}$, and once the units agree so do $C_m$ and $C'_m$. The unit $s$ is read from $Y_m/\pi^{g_m} \bmod \pi^3$, which is determined when $P_m - g_m \ge 3$, that is, $Q \ge v_m + v_{m+1} + 3$. Since $v_m \le v_{m+1} - 1$, this follows from $2v_{m+1} + 2 \le Q$. (For $\delta \ge 2$ it follows from $2v_{m+1} + \delta \le Q$ as soon as $g_m \ge 1$.) The quotient is a function of the class of $A_m C_m^{-1} \bmod \pi^{g_m+\delta}$. The rounding in $\pi^{g+\delta}\mathbb{Z}[i]$ commutes with translation by the lattice, so the computed representative depends on the class only. This class needs $A_m$ and $C_m$ mod $\pi^{g_m+\delta}$, available when $P_m - g_m \ge g_m + \delta$, that is, $Q \ge 2v_{m+1} + \delta$. The symbol increment uses $k_1, k_i$ of $A_m$ and $C_m$, which depend on residues mod $16 = $ a unit times $\pi^8$, and $\epsilon(C_m, A_m)$, which depends on residues mod $\pi^4$. These are determined when $P_m - g_m \ge 8$, which follows from $Q \ge 2v_{m+1} + 7$ as above. Then $r = A_m - q C_m$ is known mod $\pi^{P_m - g_m}$, so $Y_{m+1} = r/\pi^{g_m}$ is known mod $\pi^{P_m - 2g_m} = \pi^{Q - 2v_{m+1}}$, and $A_{m+1} = C_m$ even mod $\pi^{P_m - g_m}$. This closes the induction. For the last claim: at the first $m$ with $v_{m+1} > K$, the previous step had $v_m \le K$, so $Y_m$ and $Y'_m$ agree mod $\pi^{Q - 2v_m}$ with $Q - 2v_m > K - v_m$. If $v(Y_m) > K - v_m$, then $v(Y'_m) \ge \min(v(Y_m), Q - 2v_m) > K - v_m$. $\square$

4.3 Remarks. The precision falls by $2g$ per step, one $g$ for the division by $\pi^g$ and one for the renormalisation, exactly as in SZ04 Lemma 6. Nothing in the proof uses $\delta \ge 2$, a bound on $q$, or the choice of representative beyond "a function of the class". So the lemma also holds for $\delta = 1$ and for other representative rules (for example a "positive" quadrant rule). What $\delta \ge 2$ buys is sizes, section 5. The lemma also explains the draft's perturbation test (4.4 of the draft): it recorded the valuation of $B$ before the step but compared the new gap $t$ as well, and $t$ belongs to the next remainder, which is why the margin there looked like $M - 15$.

4.4 Test (`t9.py`). States from random 48-bit inputs were perturbed by $\pi^Q X$ and $\pi^Q Z$, with $Q$ uniform in $[4, 90]$ and $X, Z$ small or 48-bit, in 3,000 trials for each of $\delta = 1, 2, 3, 4, 6$. At the first step where the two runs differ, let $D = Q - 2v_{m+1}$. The lemma says $D < \delta$ for a quotient or gap difference and $D < \max(\delta,7)$ for any difference. Observed: the largest $D$ at a quotient or gap difference was exactly $\delta - 1$ for each $\delta$, so the quotient constant is sharp. The largest $D$ at a difference of any kind was 5, so the symbol constant cannot be lowered below 6. No violations.

## 5. Sizes

5.1 Let $x_m = |A_m|$, and note $|C_m| = x_{m+1}$. From $Y_{m+1} = (A_m - q C_m)/\pi^{g_m}$, the covering radius bound $|q| \le 2^{(g_m+\delta-1)/2}$ (true for the rounding as implemented, whatever the tie-breaking), and $|C_{m+1}| = |Y_{m+1}|\,2^{-g_{m+1}/2}$, we get
$$x_{m+2} \le 2^{-(g_m+g_{m+1})/2}\left(x_m + \rho(g_m+\delta)\,x_{m+1}\right),\qquad g_{m+1} \ge \delta,$$
which is the draft's recurrence (3), with $\rho(k) \le 2^{(k-1)/2}$.

5.2 Lemma (proven). Let $\theta \in (2^{-1/2}, 1)$, $u = \sqrt2\,\theta$ and $c(g) = \rho(g+\delta)\,2^{-g/2}$, and suppose $F(g, g') = u^{-(g+g')} + c(g)\,u^{-g'} \le 1$ for all $g, g' \ge \delta$. Put $P_m = \max(x_m,\; x_{m+1}\theta^{-g_m})$. Then $P_{m+1} \le \theta^{g_m}P_m$ for every step $m$ with $g_m \ge \delta$, which is every step except the very first, where $g_0 = 1$. At the first step the same holds with an extra factor $F(1, \delta)$.

Proof. $x_{m+1} = \theta^{g_m}\cdot x_{m+1}\theta^{-g_m} \le \theta^{g_m}P_m$. Also, by 5.1, $x_{m+2}\theta^{-g_{m+1}} \le \theta^{-g_{m+1}}2^{-(g_m+g_{m+1})/2}(P_m + \rho\,\theta^{g_m}P_m) = \theta^{g_m}P_m\,F(g_m, g_{m+1})$. $\square$

With the covering bound $c(g) \le 2^{(\delta-1)/2}$, $F$ is decreasing in both gaps, so the worst case is $g = g' = \delta$. Then $w = u^{-\delta}$ solves $w^2 + 2^{(\delta-1)/2}w = 1$. Using the exact radii $\rho(m)$ for $m \le 11$ (exhaustive search, `rho.py`) instead improves $\theta$; that version is computer-assisted (floating point; the search should be checked independently, as the draft already notes). The draft's pair dynamic programme, rescaled by $\theta^{-v}$, gives the same values as the exact-radius potential. Results (`t12.py`), with the loss of $\log_2 N$ of the potential per unit of valuation in brackets:

| $\delta$ | $\theta$, covering bound (proven) | $\theta$, exact $\rho$ (computer-assisted) | pair DP |
|---|---|---|---|
| 2 | 0.9828 (0.050) | 0.9232 (0.231) | 0.9232 |
| 3 | 0.9486 (0.152) | 0.9222 (0.234) | 0.9222 |
| 4 | 0.9417 (0.173) | 0.9300 (0.210) | 0.9300 |
| 6 | 0.9486 (0.152) | 0.9486 (0.152) | 0.9486 |

At $\delta = 2$ the first-step factor is $F(1,2) = 1.105$. At $\delta = 1$ no $\theta < 1$ exists: with $g = g' = 1$ and $\rho(2) = 1$, $F(1,1) = u^{-2} + 2^{-1/2}u^{-1}$, which equals 1 at $\theta = 1$ and exceeds 1 for every $\theta < 1$. This is the draft's weak triangle obstruction.

5.3 Consequences (proven, $\delta \ge 2$). (a) At $\delta = 2$, every $A_m$ and $C_m$ has absolute value at most $1.105\,P_0 \le 1.13 \max(|A_0|, |B_0|)$, and $|Y_{m+1}| \le 2^{-g_m/2}x_m + 2^{(\delta-1)/2}x_{m+1} \le 2.7 \max(|A_0|, |B_0|)$; for other $\delta \ge 2$ the constants change but stay bounded. So no number in a run is more than a few bits longer than the inputs. (b) $P$ falls by $\theta^{g}$ at each step and $P \ge x_m \ge 1$, which gives termination and a linear step bound; this is a short proof of the draft's $\delta \ge 2$ termination, with a weaker constant. (c) Matrix entries: each step matrix has row sums at most $2^{g/2}(1 + 2^{(\delta-1)/2})$, and a run with total valuation $j$ has at most $1 + j/\delta$ steps. So the entries of $R$ have at most $j/2 + (1 + j/\delta)\log_2(1 + 2^{(\delta-1)/2}) + 1$ bits: $1.14\,j$ at $\delta = 2$ and $0.93\,j$ at $\delta = 4$. Observed at top level: $0.55\,j$ and $0.62\,j$ (section 8.2).

## 6. The recursion

6.1 Algorithm (`rec.hgcd`). The input is a state $(A, Y)$ and a target $K$. The output is $(j, R, e)$: all steps whose pending valuation is at most $K$, with $2^j(A_m, Y_m) = R\,(A, Y)$ and $e$ the sum of their symbol increments mod 4. Write $h_c = \lceil \max(\delta,7)/2 \rceil$ (4 for $\delta \le 8$), and $T_h(z)$ for reduction of both coordinates mod $2^h$, which is finer than mod $\pi^{2h}$ and keeps $A$ primary.

1. If $Y = 0$ or $v(Y) > K$, return $(0, I, 0)$. If $K$ is below a leaf size, run the steps one by one and return.
2. $K_1 = \lfloor K/2 \rfloor$. $(j_1, R_1, e_1) = \mathrm{hgcd}(T_{K_1+h_c}(A), T_{K_1+h_c}(Y), K_1)$.
3. $(A', Y') = 2^{-j_1}R_1(A, Y)$ (exact). If $Y' = 0$ or $j_1 + v(Y') > K$, return $(j_1, R_1, e_1)$.
4. One step from $(A', Y')$ at full precision: matrix $S$, gap $g$, increment $e_0$, new state $(A'', Y'')$. Put $j = j_1 + g$ and $K_2 = K - j$.
5. $(j_2, R_2, e_2) = \mathrm{hgcd}(T_{K_2+h_c}(A''), T_{K_2+h_c}(Y''), K_2)$.
6. Return $(j + j_2,\; R_2 S R_1,\; e_1 + e_0 + e_2)$.

The top level (`rec.quartic`) starts from $(A_0, \pi B_0)$ and repeats: $K = \max(v(Y), \text{bit size of } A, Y)$, call hgcd, apply $2^{-j}R$ to the full pair, add $e$, until $Y = 0$.

6.2 Correctness (proven). By induction on $K$, hgcd performs exactly the steps of the naive loop with pending valuation at most $K$. In step 2 the truncated inputs agree with $(A, Y)$ mod $\pi^{2K_1 + 2h_c}$, and $2h_c \ge \max(\delta, 7)$. So by lemma 4.1 the sub-call, which is a genuine run on the truncated pair, performs the same steps with the same matrices and increments. In step 3, $R_1$ is the true matrix, so $(A', Y')$ is the true state after those steps, at full precision. Its pending valuation is above $K_1$ by the last clause of 4.1. Step 4 is a true step. Step 5 is step 2 again, relative to valuation $j$. The middle step makes $j_1 + g > K_1$, so $K_2 < K - K_1 = \lceil K/2\rceil$, and the recursion shrinks. The final remainder of a call is never normalised inside the call. This is deliberate: its valuation and unit, and so the increment $g k_1 + s k_i + \epsilon$ attached to it, are not determined by the truncated data, so they are left to the first step of the next call, which sees the remainder at sufficient precision. With $\delta \ge 2$ the top level terminates by 5.3(b), since each round performs at least one step. At the end, $\gcd$ and symbol are as in 3.1.

6.3 The symbol costs nothing structural. Each call returns one exponent mod 4, and increments add. Unlike the integer case, no sign has to be carried, and no quotient has to be kept positive: the role of BZ10's sign bit is played by the unit exponent $s$, which lives inside $R$ and is determined by the lemma. The only price is the extra truncation slack, $2K + 8$ $\pi$-digits instead of $2K + \delta$, the counterpart of BZ10's $2k+2$ instead of $2k+1$.

## 7. Cost

7.1 Theorem (proven). For fixed $\delta \ge 2$, the algorithm of section 6 computes $\gcd(a, b)$ and $\left[\frac{a}{b}\right]$ for inputs with $n$-bit coordinates in $O(M(n)\log n)$ bit operations. This assumes, as usual, that $M(n)/n$ is non-decreasing.

7.2 Proof. Let $T(K)$ be the cost of hgcd with target $K$ on inputs of $K + h_c$ bits. A genuine run on these inputs keeps all numbers within $O(K)$ bits, by 5.3(a); this covers $(A', Y')$ and $(A'', Y'')$, and also the inputs of the sub-calls, which are genuine runs too. The matrices have $O(K)$-bit entries by 5.3(c). Step 3 is two $2\times2$ by vector products of $O(K)$-bit numbers. Step 4 is an inverse mod $2^{\lceil (g+\delta)/2\rceil}$ with $g \le K$ (Newton, $O(M(K))$) and a few products. Step 6 is two $2\times2$ matrix products. Hence $T(K) \le T(\lfloor K/2\rfloor) + T(\lceil K/2\rceil) + O(M(K))$, with $T = O(1)$ at the leaves, so $T(K) = O(M(K)\log K)$.

For the rounds, let round $r$ start at state $m_r$ with potential $P^{(r)} = P_{m_r}$. All sizes at the start of a round are at most $c_1\log_2 P^{(r)} + c_2$ bits, with $c_1 = 1 + 1/(2\log_2(1/\theta))$; this uses $v(Y_m) = g_m \le \log_2 P_m/\log_2(1/\theta)$, which holds because $x_{m+1} \ge 1$. So $K_r = O(\log P^{(r)})$, and also $K_r \ge (\log_2 P^{(r)} - 1)/(1 + \log_2(1/\theta))$. Round $r$ performs all steps with pending valuation up to $v_{m_r} + K_r$. Round $r+1$ performs at least the next one, so $v_{m_{r+2}} > v_{m_r} + K_r$, and by 5.2, $P^{(r+2)} \le \theta^{K_r}P^{(r)}$. Hence $\log P^{(r)}$ falls by a constant factor every two rounds, the round costs $O(M(K_r)\log K_r)$ form a geometric series, and the total is $O(M(n)\log n)$, since $\log_2 P^{(0)} \le n + 2$. $\square$

7.3 What the constants look like. With the proven $\theta = 0.9828$ at $\delta = 2$, $c_1 \approx 21$ and the potential falls only by a factor of about $1 - 0.024$ per two rounds. That is a valid $O(M(n)\log n)$ with a hopeless constant. With the exact radii, $\theta = 0.9232$ gives $c_1 \approx 5.3$. In the tests (section 8.2) each round of target $K = n$ consumes valuation $j \approx K$ and leaves numbers of about $0.56\,n$ bits ($\delta = 2$). That is about 0.88 bits of norm lost per unit of valuation, against the proven 0.05 and the computer-assisted 0.23. The gap between proven and observed is the same phenomenon as the 3 to 6 times gap in the draft's step bounds.

7.4 $\delta = 1$. Lemma 4.1 and the recursion apply unchanged, and in the counts $\delta = 1$ is the cheapest variant. But without $\theta < 1$ there is neither a termination proof nor the size bound 5.3(a), so the cost bound is unproven. In BZ10 the corresponding difficulty, with positive quotients, was removed by an explicit closed form for the looping case; nothing like it is known here.

## 8. Tests

8.1 Correctness (`t10.py`, all passed).

- Quotient sequences: for $\delta = 2, 3, 4$ and leaf sizes 4 and 24, on 960 random pairs of 8 to 2048 bits, the recursive algorithm's sequence of $(q, g, s)$ equals the naive loop's (both run to $Y = 0$), and so do the symbol and the gcd. For inputs up to 700 bits it also equals, as a prefix, the sequence of the draft's `alg.sym_S` up to its stop at $B = 1$, and the symbols agree.
- Gcd: 360 pairs with a planted common factor (5 to 500 bits, inputs 20 to 1000 bits) give the Euclidean gcd up to a unit, and the symbol 0 when the gcd is not a unit.
- Euler's criterion, via the known factorisation: 500 cases ($\delta = 2, 4$) with one to four prime factors of norm up to $2^{400}$, numerators up to 1200 bits, including numerators divisible by $\pi^k$ and numerators sharing a factor. Also 24 cases ($\delta = 2, 3$) with moduli the product of two primes of norm about $2^{1000}$ and 2048-bit numerators.
- Mutual agreement of the recursive, naive and `alg.sym_S` symbols on 24 random pairs of 3000 and 4096 bits ($\delta = 2, 4$).
- In `t11.py` the recursive and naive step counts are asserted equal for $\delta = 1, 2, 4$ up to 8192 bits.

8.2 Counts (`t11.py`, leaf size 24, mean of 3 random pairs up to 2048 bits, 1 pair above). Every integer product is recorded with its operand sizes and priced as $ab$ (schoolbook), $\max\cdot\min^{0.585}$ (Karatsuba) or $(a+b)\log_2(a+b)$ (FFT-like). An inverse mod $2^h$ is priced as three products of size $h$. Columns: steps; hgcd calls, leaves, middle steps, recursion depth, top-level rounds; bits of the entries of the first top-level $R$ divided by $K$, and $j/K$; recursive cost over $n^{1.585}\log_2 n$ (Karatsuba), over $n\log_2^2 n$ (FFT-like) and over $n^2$ (schoolbook); the naive loop's schoolbook cost over $n^2$; and the ratio of naive to recursive cost in the Karatsuba model.

$\delta = 2$:

| $n$ | steps | calls | leaves | mid | depth | rounds | $R$/K | $j$/K | kar | fft | sch | naive sch | naive/rec (kar) |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 128 | 87 | 30 | 18 | 11 | 3 | 7.3 | 0.548 | 0.995 | 7.67 | 31.7 | 26.2 | 6.0 | 0.64 |
| 256 | 197 | 65 | 38 | 27 | 4 | 11.0 | 0.565 | 0.993 | 7.98 | 36.2 | 30.5 | 6.5 | 0.79 |
| 512 | 397 | 127 | 69 | 58 | 5 | 10.3 | 0.554 | 0.997 | 7.45 | 37.6 | 30.9 | 6.2 | 0.97 |
| 1024 | 773 | 258 | 135 | 123 | 6 | 12.0 | 0.554 | 0.999 | 7.01 | 38.7 | 31.7 | 6.1 | 1.21 |
| 2048 | 1521 | 518 | 266 | 252 | 7 | 13.7 | 0.551 | 1.000 | 6.52 | 39.8 | 31.8 | 6.0 | 1.54 |
| 4096 | 3077 | 1040 | 528 | 512 | 8 | 16.0 | 0.555 | 1.000 | 6.14 | 40.9 | 32.4 | 6.0 | 2.02 |
| 8192 | 6060 | 2082 | 1049 | 1033 | 9 | 16.0 | 0.557 | 1.000 | 5.77 | 41.6 | 32.7 | 6.0 | 2.64 |

$\delta = 4$: steps 67 to 4331, $R/K \approx 0.62$, Karatsuba ratio 9.81 falling to 7.26, FFT-like ratio 39.5 rising to 52.4, naive/recursive 0.85 rising to 3.51. $\delta = 1$: steps 114 to 7521, $R/K \approx 0.54$, Karatsuba ratio 7.23 falling to 5.40, FFT-like ratio 29.7 rising to 38.4, naive/recursive 0.38 rising to 1.48 (the naive loop is cheapest at $\delta = 1$, 3.0 $n^2$).

For one 4096-bit pair at $\delta = 2$ the round sizes were 4096, 2290, 1263, 703, 384, 216, 119, 63, 35, 18, 10, 5, 3 bits: a factor of about 0.56 per round. Every round consumed $j \ge K - 4$.

Reading of the counts (tested, not proven). Calls, leaves and middle steps grow linearly in $n$, and the depth grows by one per doubling, as $T(K) = 2T(K/2) + O(M(K))$ predicts. The matrix entries stay near $0.55K$ bits, half of the proven $1.14K$. The schoolbook ratio is flat, near 32, since $M(n)\log n$ is $n^2$ up to a log in that model. The Karatsuba ratio falls slowly, as it should when lower-order terms fade. The FFT-like ratio still rises slowly (by about 1 per doubling at $\delta = 2$) but more slowly than $\log n$; this is consistent with $O(M(n)\log n)$ plus linear and leaf terms, though not proof of it. In the Karatsuba model the recursion overtakes the naive loop near 500 bits at $\delta = 2$ and 4, and near 3000 bits at $\delta = 1$. These are counts in a cost model, not timings.

## 9. Where this stands

9.1 Proven here: the truncation lemma for step S (any $\delta$), the size lemma 5.2 with the covering radius, correctness of the recursion, and the $O(M(n)\log n)$ bound for $\delta \ge 2$. The proofs are short, and they would need a careful write-up, with indices checked by a second reader, before going into the paper. In particular, the proof of 4.1 should be rechecked for the unit at the first step and for the case $Y = 0$.

Computer-assisted: the improved $\theta$ from the exact radii (floating point; the radii come from an exhaustive search not checked independently).

Tested only: everything in section 8, and the sharpness of the constants in 4.1.

Unproven or open: $\delta = 1$ termination and cost; any good constant in the cost bound, since the proven contraction is far from the observed one; whether the symbol slack 7 can be lowered to 6 (tests show 5 is not enough).

9.2 Against $\mathbb{Z}$. The Gaussian recursion is simpler than BZ10 in one way and harder in another. It is simpler because no sign is needed and centred quotients work: the symbol is carried by a mod-4 exponent and the units in $R$. It is harder because one digit of quotient precision ($\delta = 1$) gives no contraction, which forces $\delta \ge 2$. The contraction constant then has to be taken from the 2-dimensional lattice geometry, and the provable constant is poor.

9.3 Before a paper. Read Weilert's Math. Comp. 2006 and JNT 2002 papers (section 2.4). Prove a better contraction: the gap between the proven 0.05 and the observed 0.88 bits per unit of valuation suggests that a pair or triple potential, or a different representative rule, could give a usable constant. Settle $\delta = 1$, or else show that $\delta = 2$ costs only a constant factor. A real implementation (GMP-level) would be needed before any claim about speed.
