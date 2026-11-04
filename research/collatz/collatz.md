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

## 3. Search

(in progress)
