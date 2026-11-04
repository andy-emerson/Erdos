# Gaussian Collatz: literature, theory, search

Working notes. Code in this directory. Nothing committed.

## 1. Literature

| work | setting | rule | what was found |
|---|---|---|---|
| Joseph, Fibonacci Q. 36 (1998) 309-317 | Z_2[i] (Gaussian 2-adics) | T(z) = z/2 on class 0 mod 2; (3z+1)/2, (3z+i)/2, (3z+1+i)/2 on classes 1, i, 1+i | T restricted to Z_2 is the 3x+1 map; T is chaotic (Devaney) and topologically (not algebraically) conjugate to T x T. No orbits on Z[i] computed. |
| Kucinski (2000), unpublished preprint, cited by Kionke | Z[i] | "similar observation" to Kionke's (divergent cones) | not found online |
| Leigh (1981), cited by Siegel as first to consider Collatz-type maps on rings of algebraic integers; Matthews followed | O_K | Matthews-type maps | details not found; Siegel reports Matthews knew no other work on O_K |
| Matthews, chapter in Lagarias (ed.) The Ultimate Challenge (2010) | Z[sqrt 2], F_p[x] | C(z) = z/sqrt2 or (3z+1)/sqrt2 | conjectured behaviour of divergent orbits (annotation in Lagarias bibliography II) |
| Kionke, arXiv:1511.05893 (2015) | lattices Z^e, Z[sqrt 2] | T(x) = (m_w x + r_w)/d per coset of Lambda/d Lambda, m_w positive integers | divergent orbits with positive lower density from "tame cones"; for Z[sqrt 2] at least half of all points diverge; for d >= 3 can have density-one finite stopping time yet positive-density divergence, so the naive multidimensional Collatz conjecture fails. Integer multipliers only, no Gaussian multiplication. |
| Siegel, arXiv:2111.07882 (withdrawn, error in sec. 3); (p,q)-adic analysis, arXiv:2412.02902 | O_K, Z^d ("Hydra maps") | general | functional-equation / Fourier framework; no Z[i] examples |
| Dumont and Reiter (2001); Pe (2004); Chamberland (1996) | C, R | analytic interpolations of 3x+1 | fractals, basins; not integral maps |
| Garg, Zenodo 10.5281/zenodo.19078383 (March 2026) | Z[i], really Z[1/2][i] | four classes mod (1+i) (sic), affine rule per class, "Variant E" uses z/4 | a length-40 periodic orbit with dyadic denominators, fractal "stability islands", Lyapunov numerics. Self-published; maps not integral on Z[i]; formulas not visible without the PDF (zenodo not reachable here). |
| Aliyev, NNTDM 31 (2025) | Z, Q | rational cycles of (p_i x + k_i)/q | lists a Gaussian-integer version as future work |
| Hicks-Mullen-Yucas-Zavislak (2008); Behajaina-Paran, Disc. Math. 348 (2024) | F_q[x], R[x], completions | polynomial Collatz | all orbits eventually periodic in positive characteristic |
| Bruschi arXiv:0810.5169; Rorabaugh (2010) | Z | base-b and Ax+B variants | Z only |

Conclusion so far: no published work studies an integral map on Z[i] that divides by 1+i and asks whether all orbits converge. Joseph is theoretical and on the 2-adic completion with multiplier 3 (which, see below, diverges on average on Z[i]). Kionke's lattice paper is the closest rigorous work and warns that in dimension 2 divergence can occur on a positive-density cone even when the average drift is negative; that applies to maps with real (scalar) multipliers, and a Gaussian multiplier rotates, which destroys invariant cones. That is a point worth checking numerically.

## 2. Theory

Notation: z is odd if (1+i) does not divide z. Accelerated map on odd z: S(z) = (alpha z + beta(z)) / (1+i)^v, v the full (1+i)-valuation, so S(z) is odd again. Each division by (1+i) halves the norm, the affine step multiplies it by about N(alpha).

If beta(z) is chosen from the class of z mod (1+i)^k so that (1+i)^k always divides alpha z + beta, then for a random odd z the excess valuation v - k is geometric, P(v - k >= j) = 2^-j, mean v = k + 1. Log-drift of the norm per odd step:

  E[ln N] change = ln N(alpha) - (k+1) ln 2,  contracting iff N(alpha) < 2^(k+1).

| k | odd classes mod (1+i)^k | contracting alpha (N(alpha) < 2^(k+1)), alpha odd |
|---|---|---|
| 1 (single beta) | 1 | N < 4: units only, so every non-trivial single-rule map diverges on average |
| 2 | 2 (1, i mod 2) | N < 8: N = 5 (2+i, 1+2i and associates) |
| 3 | 4 (the four units) | N < 16: N = 5, 9 (alpha = 3), 13 (3+2i) |
| 4 | 8 | N < 32: also 17, 25 (5, 4+3i), 29 |

Odd norms are 1 mod 4, so N(alpha) in {1, 5, 9, 13, 17, 25, 29, ...}.

The k = 3 case is canonical. The four units are a complete set of odd residues mod (1+i)^3 = 2+2i, so there is exactly one unit u(z) with alpha z + u = 0 mod (1+i)^3. The resulting map

  G_alpha(z) = z/(1+i) if (1+i) | z,  else alpha z + u(z),

is equivariant under units (G(eps z) = eps G(z)), so it descends to associate classes, the choice of 1+i versus 1-i is immaterial, and G_{eps alpha} = eps G_alpha, G_{conj alpha} = conj G_alpha up to units. Up to these symmetries the contracting k = 3 maps are exactly alpha in {1, 2+i, 3, 3+2i}, with drifts ln(5/16) = -1.16, ln(9/16) = -0.58, ln(13/16) = -0.21 (alpha = 1 is trivial: N decreases every step). alpha = 3 gives drift (3/4)^2 in norm per odd step, the exact square of the 3x+1 ratio 3/4. G_3 restricted to the real axis (mod sign) is x -> (3x + u)/2^v with u = +-1 chosen so 4 | 3x+u, i.e. ordinary 3x+1 applied to the representative of +-x that is 1 mod 4.

Joseph's map (divide by 2, three non-zero classes mod 2, multiplier alpha): drift per step (3/4) ln N(alpha) - ln 4, contracting iff N(alpha) < 4^(4/3) = 6.35, so alpha = 3 diverges on average (drift +0.26) and only N(alpha) = 5 contracts.

Worst case versus average. With (1+i)^k forced, each odd step multiplies |z| by at most about |alpha|/2^(k/2), so N(alpha) < 2^k gives a provable contraction (all orbits fall into a finite set; G_{2+i} is of this kind). The Collatz-like window, expanding in the worst case but contracting on average, as 3x+1 is (3 > 2 but 3 < 4), is

  2^k < N(alpha) < 2^(k+1).

| k | window | odd norms in window | alpha up to units |
|---|---|---|---|
| 1 | (2, 4) | none | none: no single-rule Gaussian 3x+1 exists |
| 2 | (4, 8) | 5 | 2+i, 2-i (two classes mod 2, digit choice not unit-equivariant) |
| 3 | (8, 16) | 9, 13 | 3, 3+2i, 3-2i |
| 4 | (16, 32) | 17, 25, 29 | 4+-i, 5, 4+-3i, 5+-2i |
| 5 | (32, 64) | 37, 41, 45, 49, 53, 61 | 11 classes |
| 6 | (64, 128) | 65, ..., 125 | 28 classes |

Random-walk model. Per odd step, log2 N changes by log2 N(alpha) - v with v - k geometric (mean 1, variance 2). For alpha = 5+2i, k = 4 this is drift -0.142 bits, variance 2, so P(max excursion > h bits) is about e^(-0.142 h); over 4*10^7 starts the model predicts a record excursion of ln(4*10^7)/0.142 = 123 bits, and the computation (below) found 122.9 (including the 4.86-bit pre-division peak). The model is accurate.

Divergent cones. If lambda = alpha/(1+i)^k is real up to a unit, the map with exactly k divisions is a scalar map and behaves like Kionke's lattice maps: there are positive-density sets of orbits that diverge, locked onto fixed directions. This happens for alpha = 5 at k = 4 (lambda = -5/4) and alpha = 9, 11 at k = 6 (lambda in iR), and every digit set tried for these alpha has divergent orbits. When lambda is not real up to a unit the direction keeps rotating and nothing diverged in any test. (alpha = 3 at k = 3 and alpha = 7 at k = 5 have lambda^2 real up to a unit and still converge, so the condition is on lambda itself.)

## 3. Search

Code: gcz.c (full classification of all z with N(z) <= B: memoised orbits, 128-bit coordinates, Brent cycle detection, rule given as a table over residues mod 4 or mod 8), gstop.c (stopping-time induction over one associate per class, for unit-equivariant rules), rules.py (rule tables, digit rules), exact.py (independent big-integer iteration; also re-derives every reported cycle), sweep_gen.py (non-equivariant digit sweeps), sweep_eq.py / sweep_eq2.py (unit-equivariant sweeps), joseph.py (divide-by-2 maps). Cycles are reported modulo units for equivariant rules ("classes"), with conjugate pairs counted separately; "odd" is the cycle length in odd steps.

### 3.1 Canonical unit-digit maps (k = 3)

G_alpha(z) = z/(1+i) if even, else alpha z + u, u the unit with alpha z + u = 0 mod (1+i)^3. All 3.14M starts with N(z) <= 10^6:

| alpha | drift | cycle classes (min element, odd length, share of starts) | max steps | max excursion (log2 N) |
|---|---|---|---|---|
| 2+i | provable contraction | 1 (fixed point 1) | 59 (at 10^4) | 3 |
| 3 | ln(9/16) | 2+i (2, 92.3%), 5+2i (2, 5.6%), 19+6i and conjugate (5, 1.0% each), 1 (1, 0.2%) | 269 | 7.2 |
| 3+2i | ln(13/16) | 1 (1, 63.4%), 3 (4, 22.9%), 7 (4, 12.8%), 8+7i (4, 0.8%) | 1474 | 28.6 |

So the most natural analogue, "3z + unit", converges on every start tested but has five cycle classes; it is the Gaussian analogue of 3x+1 on all of Z (which has the cycles 1, -1, -5, -17), not of 3x+1 on N. On the real axis G_3 is ordinary accelerated 3x+1 applied to whichever of +-x is 1 mod 4, and there the only cycle is 1.

Other unit-closed digit sets at k = 3 (units times 2+i, 2-i or 3): 5 to 14 cycle classes; none has one.

### 3.2 Non-equivariant digit rules, B = 10^5

Digit per class chosen freely from odd elements of norm <= 5.

| family | rules | all starts converge | exactly one nonzero cycle |
|---|---|---|---|
| k = 2, alpha in associates of 2+-i | 288 | 288 | 68 |
| k = 3, alpha associates of 3, 3+-2i | 972 | 972 | 108 |
| k = 4, N(alpha) in {17, 25, 29} | 448 | 384 (all failures are alpha = +-5, +-5i) | 76 |

A single cycle is common once the digits may break the unit symmetry, so on its own it does not single out a rule.

### 3.3 Unit-equivariant digit rules, k = 4, 5, 6 (B = 10^5, then 10^6)

k = 4, digits units together with units times (2+i) [D_A] or units times (1+2i) [D_B]; up to symmetry 7 maps:

| alpha (with D_A) | cycle classes |
|---|---|
| 4+i | 2 |
| 4-i | 2 |
| 5 | 36+ and divergent orbits (10.5% unresolved at 10^6) |
| 4+3i | 3 |
| 4-3i | 6 |
| 5+2i | 1 |
| 5-2i | 3 (incl. a cycle of 70 odd steps through 16+15i) |

k = 5 (unique minimal digit set: units, 3, 1+2i, 1-2i times units), 11 alpha: 2 to 18 classes, no divergence (alpha = 7 has 18 classes, no single-cycle map). alpha = 6+5i (drift ln(61/64)) has a cycle of 130 odd steps through 199+164i, and excursions of 2^112.

k = 6 (8 unit orbits of digits, 2 choices in two of them, 4 digit sets), 28 alpha, 112 maps (56 up to conjugation): 22 maps have a single class (alpha = 7+-4i, 8+-3i, 9+-2i, 10+-i, 10+-3i, and 11+-2i, the last with a few unresolved starts at the step cap since its drift is only ln(125/128)); alpha = 9 and 11 diverge. So single-cycle equivariant rules become common as k grows; the smallest k with one is 4, and there it is unique.

Re-run 4 Nov (sweep_eq2.py, k = 3..6, B = 10^5 and 10^6, cap 50000): all counts identical at both bounds. k = 4, alpha = 5: 34 classes among resolved starts, 10.5% unresolved (the "36+" above is not reproduced with this cap). k = 5, alpha = 6+5i has 3 classes (the 130-cycle goes through 164+199i; 199+164i is on the 6-5i cycle). k = 6: 22 of 112 single-class confirmed; for 11+-2i the gcz "unresolved" starts are escapes past 2^100 coordinates; gstop from norm 1 to 10^6 (escape 2^110) plus exact big-int iteration of the 26 escapes shows every start drops below its norm, so a single class to 10^6. Distribution up to conjugation of the 45 multi-cycle k = 6 rules: 2:19, 3:4, 4:3, 5:5, 6:1, 7:1, 8:4, 9:2, 10:2, divergent (9, 11):4. Level 2: no unit-equivariant rule exists (Prop. 1 of the paper), not even projectively. G_3 and G_{3+2i} re-verified to 10^9 (logs/G3_k3.stop, logs/G3p2i_k3.stop).

### 3.4 Joseph-type maps (divide by 2), B = 10^6

z/2 on 2Z[i], (alpha z + beta_c)/2 on the classes 1, i, 1+i mod 2. Joseph's own map (alpha = 3, beta = 1, i, 1+i) has 98% of starts unresolved at 10^5 (diverging, as the drift +0.26 predicts). With alpha = 1+2i (drift -0.18) all 16 small-beta choices converge and 8 have a single cycle (e.g. beta = 1, i, -1+i: one cycle, through 1, of 6 steps); with alpha = 2+i none does (3 or 4 cycles).

### 3.5 Explicit cycles (first-quadrant associates, accelerated map, (1+i)-valuation of each step)

| map | cycles |
|---|---|
| G_3 | 1 (v = 4); 2+i, 1+2i (3, 3); 5+2i, 2+5i (3, 3); 19+6i, 10+19i, 22+7i, 11+22i, 25+8i (3, 3, 3, 3, 4) and its mirror image 6+19i, ... |
| G_{3+2i} | 1 (3); 3, 1+4i, 2+5i, 2+3i (3, 3, 5, 4); 7, 2+9i, 8+3i, 11+2i (3, 4, 3, 5); 8+7i, 12+7i, 17+6i, 10+13i (3, 3, 4, 5) |
| H = (5+2i, units U units(2+i), k = 4) | 1 only: (5+2i) + (-1+2i) = 4+4i = -(1+i)^5, so H(1) = -1 |

G_3 on the real axis has only the cycle 1, and its basin share falls with B (0.2% at 10^6, 0.07% at 10^7), as expected for a basin that is essentially a lower-dimensional set.

### 3.6 Large-bound verification

Method: gcz classifies every z with N(z) <= 10^7 completely (orbit to its cycle). gstop then checks, for one associate of every odd z in (10^7, B], that the orbit reaches a norm below N(z) and records any orbit that returns to an associate of its start. Since the maps are unit-equivariant, this inductively proves that every orbit with N(z) <= B ends in one of the listed cycles. Arithmetic: 128-bit coordinates with escape at 2^110; the largest coordinates seen were about 2^80. Extreme orbits were re-run in exact Python big-integer arithmetic with an independent digit selection, and they agree.

| map | bound on N(z) | starts (one per associate class) | new cycles | max stopping time (T-steps) | mean | max excursion log2(N_max/N(z)) |
|---|---|---|---|---|---|---|
| H (5+2i) | 3*10^9 | 1.18*10^9 | none | 13913 (6742+34011i) | 53.9 | 122.9 (17467+5226i) |
| G_3 | 10^9 | 3.93*10^8 | none (5 classes) | 196 | 9.2 | 7.9 |
| G_{3+2i} | 10^9 | 3.93*10^8 | none (4 classes) | 2426 | 22.5 | 43.4 |

(Per-chunk results are in logs/alpha5p2i_k4.stop. The run to 3*10^9 took about 15 minutes on one core.)

H's record excursion matches the random-walk prediction (drift -0.142 bits per odd step, variance 2, so the expected record over n starts is about ln(n)/0.142 bits). H's orbits are also long: the full orbit of 6827+4464i takes 2042 odd steps to reach 1.

### 3.7 Why alpha = 5 diverges

Every divergent alpha = 5 orbit sampled (176 of about 1500 random starts of norm up to 1.8*10^7) lands on the lines x - y = +-1 or x + y = +-1. These lines are nearly invariant (876 of 1000 sample points map back onto them). On them the map acts like a one-dimensional "5x + c" map, whose drift ln 5 - 2 ln 2 > 0 is positive, and the orbits leave along the 45-degree directions with ratio exactly -5/4 per step. This is the integer-line version of Kionke's divergent cones. It needs lambda = alpha/(1+i)^k to be real up to a unit, which is why only 5 (k = 4) and 9, 11 (k = 6) show it.

## 4. Assessment

Known: no prior computation of any integral Collatz-type map on Z[i] with (1+i)-division. Joseph's Z_2[i] map (multiplier 3, divide by 2) is theory only, and on Z[i] it diverges for most starts. Kionke treats scalar-multiplier lattice maps and proves positive-density divergence for some of them. Leigh and Matthews work on O_K but give no Gaussian cases. Garg's 2026 Zenodo note is non-integral.

Most Collatz-like rules found:

1. H: z -> z/(1+i) if (1+i) | z; else (5+2i)z + d with d in {+-1, +-i, +-(2+i), +-(1-2i)} the unique digit making the result divisible by (1+i)^4. It is unit-equivariant, one cycle (the fixed class of 1), and verified for all N(z) <= 3*10^9. It sits in the Collatz window (29/32 per odd step on average, expanding 29/16 in the worst case), with long orbits and huge excursions, so it is non-trivially Collatz-like. Among unit-equivariant rules with minimal-norm digit sets and k <= 5, it is the only single-cycle one (up to units and conjugation). At k = 3 the extra digit sets units*(2+-i) and units*3 were also tried, and none of them has a single cycle either.
2. G_3 (3z + unit, k = 3): the most natural analogue. It converges on everything to 10^9, with exactly five cycle classes (counting the mirror pair separately) and fast stopping times. It plays the role of 3x+1 on all of Z.
3. Divide-by-2 maps with alpha = 1+2i (Joseph form, beta = 1, i, -1+i): one cycle at 10^6, also unit-free and less canonical.

What would be publishable:

(a) A short paper: "Collatz maps on the Gaussian integers". It would contain the window theorem (2^k < N(alpha) < 2^(k+1), and no single-rule analogue exists), the classification of unit-equivariant digit maps for k <= 5 (finite lists, done here), complete cycle lists verified to 10^9 (G_3: 5 classes; G_{3+2i}: 4; H: 1), the conjecture that H has a single cycle, the divergence criterion (lambda real up to a unit) with the invariant-line mechanism, and the random-walk fit for excursions and stopping times. The territory is empty, so the computation itself is new.
(b) To strengthen it: push H to 10^11 or 10^12 (gstop runs at about 1.2*10^6 starts per second per core, so 10^11 is about 9 core-hours); prove the divergence for alpha = 5 rigorously (the invariant-line argument looks tractable); prove a Terras/Everett-type density-one finite stopping time for these maps (the 2-adic argument should carry over to (1+i)-adics, since the parity vector of the first n steps depends only on z mod (1+i)^(n+k) or so); and check for cycles of H by the Steiner/Simons-de Weger method (a cycle with n odd steps and total valuation V needs 2^V close to 29^n with a unit-phase condition; these bounds might rule out cycles with few odd steps).
(c) Caveats: a single cycle at k = 6 is common (22 of 112 maps), so the claim "H is special" rests on minimality (smallest k) and uniqueness at that k, not on single-cycleness itself. Non-equivariant rules with a single cycle are also common (about 10% of small rules at k = 2..4).

## Sources

- Joseph, Fibonacci Quarterly 36(4) (1998): https://www.mathstat.dal.ca/FQ/Scanned/36-4/joseph.pdf
- Kionke, A geometric approach to divergent points of higher dimensional Collatz mappings: https://arxiv.org/abs/1511.05893
- Lagarias, annotated bibliographies: https://arxiv.org/abs/math/0309224 , https://arxiv.org/abs/math/0608208 ; overview https://arxiv.org/abs/2111.02635
- Siegel, (p,q)-adic analysis and the Collatz conjecture: https://arxiv.org/abs/2412.02902 ; withdrawn https://arxiv.org/abs/2111.07882
- Garg (2026), Zenodo 10.5281/zenodo.19078383 (summary via https://synapsesocial.com/papers/W7139080527)
- Behajaina and Paran: https://arxiv.org/abs/2312.00390
- Aliyev, NNTDM 31(3) (2025): https://nntdm.net/papers/nntdm-31/NNTDM-31-3-658-666.pdf
- Matthews, generalized 3x+1 mappings: https://arxiv.org/abs/1907.13302 , https://arxiv.org/abs/1910.11798
