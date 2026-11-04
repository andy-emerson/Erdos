# Verifying the Gaussian Goldbach conjectures to norm $10^{10}$

*Draft research note, 4 November 2026. Code and outputs: `research/goldbach/`. Items marked [unverified] have not been checked against their source or by computation.*

## Abstract

In 1968 Holben and Jordan proposed three Gaussian analogues of the binary Goldbach conjecture: every even Gaussian integer is a sum of two Gaussian primes (D), and, above small thresholds, such a sum exists with both primes within $45^\circ$ (E) or $30^\circ$ (F) of the target. In 2016 Knill proposed a quadrant version (Q): every even $a+bi$ with $a, b > 1$ is a sum of two Gaussian primes with positive real and imaginary parts. We checked all four for every even Gaussian integer $z$ with $N(z) \le 10^{10}$, which is $15{,}707{,}962{,}441$ targets in the plane. No exception exists in that range. Read literally with a strict inequality, E fails at $2$ and $3+i$ (and their images under units and conjugation), as Holben and Jordan themselves remarked; with $\le 45^\circ$ it holds for all $N(z) > 2$. For each target the program finds the representation with the smallest first prime. The largest such minimum is $N(p_1) = 1301$ for E and $1949$ for F, and the hardest cases of Q all lie on the row $b = 2$, where Q becomes a Goldbach problem for primes of the form $n^2+1$. The run took 10.3 minutes on one core.

## 1. Introduction

Goldbach's conjecture says that every even integer greater than $2$ is a sum of two primes. It has been checked by computer for every even number up to $4 \cdot 10^{18}$ by Oliveira e Silva, Herzog and Pardi [7], after earlier verifications to $10^{14}$ by Deshouillers, te Riele and Saouter [8] and to $4 \cdot 10^{14}$ by Richstein [9] [unverified: [8] and [9] are cited from memory and not yet checked against the papers].

The Gaussian integers are the numbers $a+bi$ with $a, b \in \mathbb{Z}$. Their norm is $N(a+bi) = a^2+b^2$, and the units are $\pm 1$ and $\pm i$. A **Gaussian prime** is a Gaussian integer whose only divisors are units and its own unit multiples. They are of two kinds:

- $x+yi$ with $xy \ne 0$ is prime exactly when $x^2+y^2$ is a rational prime (this rational prime is $2$ or is $1 \bmod 4$);
- $x+yi$ with $x = 0$ or $y = 0$ is prime exactly when the nonzero coordinate is $\pm q$ for a rational prime $q \equiv 3 \pmod 4$.

So $3$, $2+i$, $1+i$ and $5+2i$ are Gaussian primes, while $2 = -i(1+i)^2$ and $5 = (2+i)(2-i)$ are not.

The prime $1+i$ plays the part of $2$. The ring $\mathbb{Z}[i]/(1+i)$ has two elements, so every Gaussian integer is either divisible by $1+i$ or congruent to $1$ modulo it. We call $z$ **even** if $1+i$ divides $z$; this happens exactly when $a+b$ is even, since $(a+bi)/(1+i) = \tfrac{1}{2}\left((a+b) + (b-a)i\right)$. Every Gaussian prime other than the four associates of $1+i$ is odd, and the sum of two odd Gaussian integers is even. The question is whether every even Gaussian integer arises this way, and in how restricted a way.

Little computation on this question has been published. Holben and Jordan checked E and F for all even targets in the square $|a|, |b| \le 50$, which is complete only up to norm $2500$. Knill shows pictures of representation counts in a small box. We found no published verification with a stated bound [unverified: based on a search of the literature, OEIS and question sites, which may have missed something].

## 2. The conjectures

For nonzero $p$ and $z$ let $\theta(p, z) \in [0^\circ, 180^\circ]$ be the angle between them as vectors in $\mathbb{R}^2$. Holben and Jordan [1] stated three conjectures, here with the thresholds as norms:

- **D.** Every even Gaussian integer $z$ is $p_1 + p_2$ with $p_1, p_2$ Gaussian primes.
- **E.** Every even $z$ with $N(z) > 2$ is $p_1 + p_2$ with $\theta(p_1, z) < 45^\circ$ and $\theta(p_2, z) < 45^\circ$.
- **F.** Every even $z$ with $N(z) > 10$ is $p_1 + p_2$ with $\theta(p_1, z) < 30^\circ$ and $\theta(p_2, z) < 30^\circ$.

The scan of [1] is partly garbled. The threshold in E prints as something like "$|a| > \sqrt{2}$", which Knill and later authors read as $N(z) > 2$; we use that reading [unverified: the scan was read through OCR].

Knill [4] stated a quadrant version:

- **Q.** Every even $z = a+bi$ with $a > 1$ and $b > 1$ is $p_1 + p_2$ with $p_1, p_2$ Gaussian primes in the open first quadrant $\{x+yi : x > 0,\ y > 0\}$.

D alone is weak in the plane. If $p$ is a Gaussian prime then so is $-p$, so a sum $p_1 + p_2$ may really be a difference: two large primes pointing in nearly opposite directions can cancel to leave a small $z$. In the integers, the analogous statement would be "every even number is a sum or difference of two primes", which is far weaker than Goldbach. Holben and Jordan call D "a poor generalization" for this reason. What makes ordinary Goldbach hard is that the two primes are positive, so both are smaller than the target and there are only finitely many candidates.

The angle conditions bring this back. If both $p_1$ and $p_2$ are within $45^\circ$ of $z$, they lie in the open quarter plane centred on the direction of $z$, and neither can cancel the other. Each has a positive component along $z$, these components add up to $|z|$, so each is less than $|z|$, and therefore $|p_j| < \sqrt{2}\,|z|$. As in the integers, only finitely many pairs are possible, and the question is whether one of them works. F narrows the cone to $60^\circ$ in all. Since E and F ask for a representation of a special kind, each is a stronger claim than D: E implies D for $N(z) > 2$, and F implies E for $N(z) > 10$. D for the remaining even $z$ with $N(z) \le 2$ is checked directly: $0 = (2+i) + (-2-i)$ and $1+i = (3+2i) + (-2-i)$, and the other associates of $1+i$ follow by multiplying by units.

The angle cannot be reduced to $0^\circ$. A Gaussian prime pointing along the positive real axis is a rational prime $q \equiv 3 \pmod 4$, and the sum of two such primes is $2 \bmod 4$, so $4, 8, 12, \dots$ have no representation with both angles zero.

Q uses a fixed cone, the first quadrant, in place of a cone centred on $z$. Its strength shows near the edges of the quadrant. On the row $b = 2$ the imaginary parts of $p_1$ and $p_2$ must both be $1$, so $p_1 = n+i$ and $p_2 = m+i$ with $n + m = a$, and $n+i$ is prime exactly when $n^2+1$ is a rational prime. Q on this row says that every even $a \ge 2$ is $n + m$ with $n, m \ge 1$ and both $n^2+1$ and $m^2+1$ prime. This would give infinitely many primes of the form $n^2+1$ (Landau's problem), as Knill notes, because a finite set of such $n$ has bounded sums.

### 2.1 What "verified" means here

We say a conjecture is verified to norm $10^{10}$ when, for every even $z$ in its range with $N(z) \le 10^{10}$, the program found Gaussian primes $p_1, p_2$ with $p_1 + p_2 = z$ that meet its condition, using exact integer arithmetic throughout. Each claim is about a found representation; no target is skipped by a heuristic.

### 2.2 Strict and non-strict angles

Holben and Jordan write "$< 45^\circ$" and "$< 30^\circ$". Later authors sometimes write $\le$; Giannitsi et al. [5] state F with $\le \pi/6$. For $30^\circ$ the two readings agree: an angle of exactly $30^\circ$ between integer vectors would need $\tan^2 = 1/3$ as a ratio of squares of integers, which is impossible. For $45^\circ$ they differ, since $\tan 45^\circ = 1$ is attained.

The strict form of E fails at exactly two targets up to symmetry, $2$ and $3+i$ (together with $1+3i$ and the other images under units and conjugation). Both have representations with both angles exactly $45^\circ$: $2 = (1+i) + (1-i)$ and $3+i = (1+2i) + (2-i)$. Holben and Jordan remark that $2$, $3+i$ and $1+3i$ "require $45^\circ$", which is why F has the threshold $N(z) > 10$. Their E is therefore best read with $\le 45^\circ$ for $N(z) > 2$, or with $< 45^\circ$ for $N(z) > 10$; we verified both.

## 3. Method

The program is `gg.c` (C, about 250 lines, no outside libraries).

### 3.1 Primality from norms

By the description of Gaussian primes in Section 1, testing whether $x+yi$ is prime needs only a rational primality test of $x^2+y^2$ (off the axes) or of $|x|$ or $|y|$ with a check mod $4$ (on the axes). A table of rational primes therefore serves as a table of Gaussian primes, and there is no need to factor $p \equiv 1 \pmod 4$ in $\mathbb{Z}[i]$.

Targets are processed in shells of norm $[S, S + 2^{27})$. For each shell the program runs a segmented odd-only sieve of Eratosthenes over the norms that the shell can reach, the shell plus a margin of $2|z|_{\max}\sqrt{P} + P$ on each side, where $P = 2 \cdot 10^6$ bounds the norm of the candidate $p_1$. A norm outside the sieved segment goes to a fixed table of primes below $2^{24}$ or else to a Miller–Rabin test with the first twelve primes as bases, which is exact for all 64-bit inputs. Correctness therefore does not depend on the margin. In every run reported here the Miller–Rabin fallback was never called.

### 3.2 Symmetry

Multiplying by a unit and taking complex conjugates map Gaussian primes to Gaussian primes, even numbers to even numbers, and preserve angles. These eight maps carry every target to one with $0 \le b \le a$, so E and F need only that domain. For Q, the map $z \mapsto i\bar{z}$ sends $a+bi$ to $b+ai$ and the open first quadrant to itself, so Q needs only $2 \le b \le a$. The domain holds about $\pi N / 16$ even targets of norm at most $N$, one eighth of the plane.

### 3.3 Minimal first prime

For E and F the candidates $p_1$ are all Gaussian primes of norm at most $P$ (all associates, $595{,}780$ of them), sorted by norm. For each target the program takes them in order and stops at the first $p_1$ that passes the angle test and has $z - p_1$ prime. The result is the smallest $N(p_1)$ over all valid representations, and since $p_1$ and $p_2$ can be swapped it is also the smallest value of $\min(N(p_1), N(p_2))$. Running out of candidates would be reported as a failure, which would mean only that the list was too short; it never happened.

The search is short because a small prime almost always works: $98\%$ of E targets up to $10^{10}$ have a representation with $N(p_1) < 64$, and $99.8\%$ with $N(p_1) < 128$. Most candidates are rejected by the angle test, which costs a few multiplications, before any table lookup.

For Q the candidates are $p_1 = x + yi$ with $x, y \ge 1$, taken along the diagonals $x + y = k$ for $k = 2, 3, \dots$, with $x \le a-1$ and $y \le b-1$ so that $z - p_1$ is also in the open quadrant. The result is the smallest $k$, the $L^1$ size of $p_1$. On the row $b = 2$ this is the search for the smallest $n$ with $n^2+1$ and $(a-n)^2+1$ both prime.

### 3.4 Exact angle tests

Write $p_1 \bar{z} = d + c\,i$. The real part $d$ is the dot product of $p_1$ and $z$, and $|c|$ is the absolute value of their cross product. With $p_2 = z - p_1$ the corresponding values for $p_2$ are $N(z) - d$ and $-c$. Then
$$\theta(p_1, z) < 45^\circ \iff d > |c|, \qquad \theta(p_1, z) < 30^\circ \iff d > 0 \text{ and } 3c^2 < d^2,$$
and the same with $N(z) - d$ in place of $d$ for $p_2$. The non-strict $45^\circ$ test uses $d \ge |c|$ with $d > 0$. All quantities are integers; the squares in the $30^\circ$ test are formed in 128-bit arithmetic.

## 4. Checks of correctness

The checks below were run during the work and their outcomes recorded in the working notes [unverified: the console logs were not kept; the scripts are in the directory and can be rerun].

An independent brute-force program (`ref.py`, Python with numpy) searches the whole box of candidates for each target with no early exit, writes the angle tests in the different form $2d^2 > N(p)N(z)$ and $4d^2 > 3N(p)N(z)$, and uses its own sieve. For all $4013$ even targets in the domain with $N(z) \le 20{,}000$ its minimal values for E, F and Q agree with `gg.c`, including which targets need $\le 45^\circ$.

The sieve was checked against pure Miller–Rabin (environment flag `GGNOSIEVE=1`) and against three other choices of shell width and margin, on the $196{,}335$ targets with $9.99 \cdot 10^8 < N(z) \le 10^9$: the per-target output is identical in all four. Splitting a range into two runs gives the same output as one run.

The number of targets each run reports was compared with a direct count of the lattice points $(a, b)$ with $a \equiv b \pmod 2$ and the right bounds (`check_top.py`), and agrees for every run and chunk. This guards against a shell boundary dropping or repeating targets. The count of $15{,}707{,}962{,}441$ even targets in the whole plane was recomputed separately.

For the twelve hardest targets of each variant in every run, `check_top.py` re-verifies the representation using sympy's primality test and proves the reported value minimal by testing every smaller candidate $p_1$.

## 5. Results

The range $N(z) \le 10^{10}$ was run as $[0, 10^9]$ followed by nine windows $(k \cdot 10^9, (k+1) \cdot 10^9]$ for $k = 1, \dots, 9$, and the outputs merged after checking that the windows are contiguous.

| conjecture | targets in the domain | exceptions |
|---|---|---|
| D | all even $z$ | none (follows from E, Section 2) |
| E, $\le 45^\circ$, $N(z) > 2$ | $1{,}963{,}555{,}659$ | none |
| E, $< 45^\circ$, $N(z) > 2$ | same | $2$ and $3+i$ only |
| F, $N(z) > 10$ | $1{,}963{,}555{,}656$ | none |
| Q, $2 \le b \le a$ | $1{,}963{,}455{,}660$ | none |

### 5.1 Hardest cases

For each target let $m(z)$ be the smallest $N(p_1)$ (for E and F, with the strict angle test) or the smallest $L^1$ size of $p_1$ (for Q). The targets with the largest $m(z)$ up to $10^{10}$ are:

| variant | $z$ | $N(z)$ | $m(z)$ | $p_1$ |
|---|---|---|---|---|
| E | $28708 + 28708i$ | $1{,}648{,}298{,}528$ | $1301$ | $25+26i$ |
| E | $80252$ | $6{,}440{,}383{,}504$ | $1109$ | $25-22i$ |
| E | $70511 + 70511i$ | $9{,}943{,}602{,}242$ | $1109$ | $22+25i$ |
| E | $92381 + 8611i$ | $8{,}608{,}398{,}482$ | $1033$ | $32-3i$ |
| F | $57874$ | $3{,}349{,}399{,}876$ | $1949$ | $43-10i$ |
| F | $59660 + 17554i$ | $3{,}867{,}458{,}516$ | $1861$ | $30+31i$ |
| F | $90761 + 38061i$ | $9{,}686{,}198{,}842$ | $1789$ | $42+5i$ |
| F | $36226 + 36226i$ | $2{,}624{,}646{,}152$ | $1753$ | $27+32i$ |
| Q | $90232 + 2i$ | $8{,}141{,}813{,}828$ | $1107$ | $1106+i$ |
| Q | $94872 + 2i$ | $9{,}000{,}696{,}388$ | $967$ | $966+i$ |
| Q | $82232 + 2i$ | $6{,}762{,}101{,}828$ | $907$ | $906+i$ |
| Q | $94702 + 2i$ | $8{,}968{,}468{,}808$ | $907$ | $906+i$ |

Several of the E and F records lie on the real axis or the diagonal $a = b$. On these lines of symmetry the representations come in mirror pairs, so a target has about half as many independent chances; we offer this as a likely reason, not a tested one.

All ten of the largest Q values lie on the row $b = 2$. There the only representations are $(n+i) + (m+i)$ with $n^2+1$ and $m^2+1$ prime, which is a thin set: for $z = 90232+2i$ the smallest choice is $n = 1106$, with $1106^2 + 1 = 1{,}223{,}237$ and $89126^2+1 = 7{,}943{,}443{,}877$ both prime. As a by-product, every even $a$ with $2 \le a \le 99{,}998$ is a sum $n + m$ with $n^2+1$ and $m^2+1$ prime. Rows with $b \ge 3$ allow imaginary parts other than $1$ and are much easier.

### 5.2 Growth

The record value grows slowly. The table gives the largest $m(z)$ over all targets with $N(z)$ up to each bound, from a second program (`gg_bins.c`) that keeps the maximum in each dyadic range of norms up to $10^9$, and from the merged run at $10^{10}$:

| $N(z)$ up to | $(\ln N)^2$ | E: max $m(z)$ | F: max $m(z)$ | Q: max $m(z)$ |
|---|---|---|---|---|
| $2^{12}$ | $69$ | $97$ | $101$ | $25$ |
| $2^{16}$ | $123$ | $157$ | $397$ | $75$ |
| $2^{20}$ | $192$ | $281$ | $421$ | $265$ |
| $2^{24}$ | $277$ | $557$ | $797$ | $265$ |
| $10^{9}$ | $429$ | $941$ | $1549$ | $785$ |
| $10^{10}$ | $530$ | $1301$ | $1949$ | $1107$ |

For E the ratio of the record to $(\ln N)^2$ rises from about $1.4$ to $2.5$ over this range, and for F it stays between about $1.5$ and $3.7$. The data fit growth of order $(\ln N)^2$ times a slowly increasing factor, which is the shape seen for the smallest prime in ordinary Goldbach partitions; the range is too short to separate $(\ln N)^2$ from, say, $(\ln N)^2 \ln \ln N$. For Q the right scale is $a \approx \sqrt{N}$ on the row $b = 2$, and the records there behave like Goldbach records for the sparser primes $n^2 + 1$.

### 5.3 Representation counts

For small targets `ref.py` counts all ordered pairs $(p_1, p_2)$. A selection:

| $z$ | $N(z)$ | E, $< 45^\circ$ | E, $\le 45^\circ$ | F | Q |
|---|---|---|---|---|---|
| $1+i$ | $2$ | $0$ | $0$ | $0$ | |
| $2$ | $4$ | $0$ | $2$ | $0$ | |
| $2+2i$ | $8$ | $1$ | $1$ | $1$ | $1$ |
| $3+i$ | $10$ | $0$ | $2$ | $0$ | |
| $4$ | $16$ | $2$ | $2$ | $2$ | |
| $3+3i$ | $18$ | $2$ | $4$ | $2$ | $2$ |
| $4+2i$ | $20$ | $3$ | $3$ | $1$ | $1$ |
| $6$ | $36$ | $7$ | $7$ | $5$ | |
| $10+10i$ | $200$ | $20$ | $28$ | $16$ | $20$ |
| $20$ | $400$ | $40$ | $40$ | $24$ | |

Blank cells are targets outside the range of Q. The target $2+2i = (1+i)+(1+i)$ is the only one with a single ordered pair under every condition, and F at $4+2i$ has only $(2+i)+(2+i)$. The full table for $N(z) \le 400$ is `research/goldbach/counts_small.txt`.

## 6. Related work

Mitsui [2, 3] extended the Hardy–Littlewood and Vinogradov methods to algebraic number fields. For the Gaussian integers this gives the ternary analogue: every sufficiently large odd (in the appropriate sense) element is a sum of three primes. The binary statement, of which D is the Gaussian case, is left as a conjecture [unverified: Mitsui was not read directly; this summary follows Giannitsi et al. and Knill].

Holben and Jordan [1] stated D, E and F alongside Gaussian twin, triplet and quadruplet prime conjectures, and proved there are only finitely many Gaussian prime quintuplets.

Knill [4] stated Q, a ternary quadrant version, and analogues for the Hurwitz, octonion and Eisenstein integers. He also remarks that $4+13i$ is the smallest Gaussian integer that is not a sum of two Gaussian primes and $29$ the smallest real one. These are odd targets, which can be sums only if one summand is an associate of $1+i$; we rechecked both (`odd_check.py`), the next exception after the orbit of $4+13i$ being $12+17i$.

Giannitsi, Krause, Lacey, Mousavi and Rahimi [5] proved a sector version of the ternary problem: for any arc of directions, every odd $n$ of large norm whose direction is not too close to the ends of the arc is a sum of three primes with directions in the arc. For the binary problem they proved that the even targets in a sector with no representation by primes in that sector have density zero, with a saving of any power of $\log N$. They list F as open.

Banerjee and Rahaman [6] proved a Chen-type result: every even Gaussian integer of large norm is $p + P_2$, with $P_2$ having at most two prime factors, together with a lower bound of order $N(z)/(\log N(z))^2$ for the number of such representations. Their result has no angle condition.

None of these papers reports a numerical verification beyond the small boxes of [1] and [4].

## 7. Extensions

### 7.1 Larger bounds

Measured times, on one core with the process niced and other jobs running, for E, F and Q together:

| range of $N(z)$ | CPU time |
|---|---|
| $[0, 10^8]$ | $4.5$ s |
| $[0, 10^9]$ | $54$ s |
| each window of $10^9$ between $10^9$ and $10^{10}$ | $60$ to $67$ s |
| $[0, 10^{10}]$ | $619$ s ($10.3$ min) |

The cost per target is about $340$ ns for all three variants at the top of the range, and the cost per window of $10^9$ norms rose by about $11\%$ from the first window to the last. Two things drive that growth. The search grows slowly with the minimal summands of Section 5.2. The sieve grows faster: its margin is $2|z|\sqrt{P}$ with $P = 2 \cdot 10^6$, so the sieved length per shell grows like $\sqrt{N}$. By count of sieved numbers, the sieved segment is about $5$ times the shell width at $10^{10}$, $40$ times at $10^{12}$ and $400$ times at $10^{14}$, and its memory grows to about $360$ MB at $10^{12}$. If sieving costs between $0.5$ and $1$ ns per number (assumed, not measured), the sieve takes $3$ to $5$ s of each $10^9$ window at $10^{10}$ and grows in proportion to the segment.

The margin is much larger than needed. The largest minimal $p_1$ up to $10^{10}$ has $|p_1| < 45$, so a candidate list of norm $10^4$ would cut the margin about fourteenfold, and the Miller–Rabin fallback keeps the program correct if a target ever needs more. With that change, and the reorganised sieve described below for the largest bounds, the search cost dominates. Assuming the search cost per window keeps growing by about $10\%$ per factor of ten in $N$, the estimates are:

| bound | windows of $10^9$ | estimated core-hours, search only | estimated core-hours, `gg.c` as is |
|---|---|---|---|
| $10^{12}$ | $10^3$ | about $20$ | $25$ to $35$ |
| $10^{13}$ | $10^4$ | about $220$ | $400$ to $700$ |
| $10^{14}$ | $10^5$ | about $2500$ | $8000$ to $15{,}000$ |

All of these are extrapolations [unverified]. The windows are independent, so the work splits across cores or machines with no communication. At $10^{14}$, $|z| \le 10^7$; norms stay below $2^{47}$ and fit 64-bit arithmetic, with the existing 128-bit squares in the F test. Beyond about $10^{13}$ the shell layout itself should change, for example to sieving along rows of constant $b$, because even a reduced margin then exceeds the shell width.

A result to $10^{12}$ would cover $|z| \le 10^6$ and about $1.6 \cdot 10^{12}$ even targets, and would extend the row $b = 2$ of Q to even $a \le 10^6$.

### 7.2 Certification

A bound reported by one program rests on that program. Oliveira e Silva, Herzog and Pardi [7] guarded against this by an independent second check over part of their range [unverified: we recall a double check to $4 \cdot 10^{17}$ but have not confirmed it in the paper]. The plan here is the same in outline.

A second program would be written separately, with its own source of primality: either an external rational prime generator, or a sieve in $\mathbb{Z}[i]$ that splits each $p \equiv 1 \pmod 4$ as $(x+yi)(x-yi)$ by Cornacchia's algorithm and marks Gaussian multiples directly. It would also use a different search order, for example testing $p_2$ in place of $p_1$, or scanning the cone by angle.

Both programs would write, for each shell and each variant, a checksum of the sequence of minimal values $m(z)$ in a fixed order of targets. The minimal value is a property of $z$ alone, while the $p_1$ that attains it need not be unique, so the checksum covers values only. Matching checksums for every shell, together with matching target counts, certify the run. The published record would then consist of the per-shell checksums, the target counts and the full list of record cases with their representations, which anyone can recheck with a few primality tests.

## References

[1] C. A. Holben and J. H. Jordan, The twin prime problem and Goldbach's conjecture in the Gaussian integers, *Fibonacci Quarterly* 6(5) (1968), 81–85 and 92.

[2] T. Mitsui, On the Goldbach problem in an algebraic number field I, *J. Math. Soc. Japan* 12(3) (1960), 290–324.

[3] T. Mitsui, On the Goldbach problem in an algebraic number field II, *J. Math. Soc. Japan* (1960) [unverified: volume and pages not checked].

[4] O. Knill, Goldbach for Gaussian, Hurwitz, Octavian and Eisenstein primes, arXiv:1606.05958 (2016).

[5] C. Giannitsi, B. Krause, M. T. Lacey, H. Mousavi and Y. Rahimi, Averages over the Gaussian primes: Goldbach's conjecture and improving estimates, arXiv:2309.14249 (2023).

[6] D. Banerjee and M. Rahaman, Representation of even Gaussian integer à la Chen, arXiv:2406.16311 (2024).

[7] T. Oliveira e Silva, S. Herzog and S. Pardi, Empirical verification of the even Goldbach conjecture and computation of prime gaps up to $4 \cdot 10^{18}$, *Math. Comp.* 83 (2014), 2033–2060.

[8] J.-M. Deshouillers, H. J. J. te Riele and Y. Saouter, New experimental results concerning the Goldbach conjecture, *ANTS-III*, LNCS 1423 (1998), 204–215 [unverified].

[9] J. Richstein, Verifying the Goldbach conjecture up to $4 \cdot 10^{14}$, *Math. Comp.* 70 (2001), 1745–1749 [unverified].
