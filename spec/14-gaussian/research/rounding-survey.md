# Quotient conventions for Gaussian division with remainder: survey

Scope: every way found to pick the quotient q in x = q*y + r for Gaussian integers, with the integer conventions they generalize, judged on uniqueness, the norm bound, symmetry under units and conjugation, behaviour on the real axis, and integer-only cost. Light web survey, November 2026. Items marked "not verified online" are cited from standard bibliographies or memory; their pages were not fetched in this pass.

Notation used throughout: x = a + bi, y = c + di, N = N(y) = c^2 + d^2, A = ac + bd, B = bc - ad, so x/y = z = (A + Bi)/N. Every convention computes some q from z; then r = x - q*y = y*(z - q) and N(r) = N(y) * N(z - q). The fractional part f = z - q decides everything.

## 1. Integer conventions (the one-dimensional case)

Boute and Leijen define the "basic division conditions" as D = d*q + r with |r| < |d| and q an integer; the conventions differ only in which of the (at most two) admissible q they take.

| Name | q | r range | Who uses it |
|---|---|---|---|
| T, truncated | trunc(D/d) | sign of D, abs < abs(d) | C99, most CPUs, Haskell quot/rem |
| F, floored | floor(D/d) | sign of d | Knuth, Python, Haskell div/mod |
| E, Euclidean | chosen so 0 <= r < abs(d) | [0, abs(d)) | Boute 1992, Leijen's LVM |
| C, ceiling | ceil(D/d) | opposite sign of d | Common Lisp ceiling |
| R, rounded | nearest integer, tie rule needed | abs(r) <= abs(d)/2 | Common Lisp round, IEEE 754 remainder (ties to even) |

Properties worth carrying to Z[i]:

- All five give a unique q; all satisfy abs(r) < abs(d). R also gives abs(r) <= abs(d)/2.
- T is odd: q(-D, d) = -q(D, d). The price is that r is not a function of the residue class of D (r(1, 3) = 1 but r(-2, 3) = -2).
- F and E make r a function of the residue class: F of (D mod d, d), E of (D mod d, abs(d)), so E depends only on the ideal (d). This is Boute's main argument for E.
- R with ties up or ties down is "F shifted by a half": q = floor(D/d + 1/2) or ceil(D/d - 1/2). The half-open interval does the tie-breaking, so no explicit tie case exists. R with ties to even or away from zero is odd like T and has T's defect at ties: r(1, 2) and r(-1, 2) have opposite signs although 1 and -1 are congruent mod 2.

A short argument shows the last trade-off is forced. If r depends only on the class of D and the rule is odd, then at D = d/2 (d even) we have D congruent to -D, so r = -r, so r = 0, which is false. Any unique convention must give up one of the two at ties.

Knuth defines x mod y = x - y*floor(x/y) (TAOCP Vol. 1, section 1.2.4; Concrete Mathematics, section 3.4; not verified online). Leijen's note proves that E and F can be computed from T with a sign fix-up.

## 2. Where ties occur in Z[i]

A part of z is a half-integer exactly when 2A is congruent to N mod 2N (or the same for B). That needs N even, which is the case exactly when (1+i) divides y. For a divisor of odd norm no tie can occur, whatever the convention. Ties are therefore a fixed, describable set, not a rounding artefact.

Two examples cover the cases:

- y = 2, x = 1: z = 1/2. The q with N(r) < N(y) are 0 and 1 (r = 1 or -1). No other q qualifies.
- y = 1 + i, x = 1: z = (1 - i)/2, a corner. The q in {0, 1, -i, 1 - i} give r in {1, -i, i, -1}, all of norm 1 = N(y)/2.

So even with the weaker requirement N(r) < N(y), two or four quotients qualify at these points. Every convention with a unique q makes a choice there, whether it is written as a tie rule or hidden in a half-open interval. Floor is not tie-free in a deeper sense; it just picks the half-open box [0, 1)^2, which is too large (section 3).

## 3. Componentwise floor, truncation, ceiling, Euclidean quadrant

Apply an integer convention to each part of z.

- Floor: f in [0, 1)^2, so N(f) < 2 and only N(r) < 2N(y). For y = 4, x = 3 + 3i: z = (3 + 3i)/4, floor gives q = 0 and r = x with N(r) = 18 > 16 = N(y). Since f can approach (1, 1), the norm-decrease proof of Euclid fails.
- Truncation and ceiling: f in (-1, 1)^2 or (-1, 0]^2, same bound N(r) < 2N(y).
- Euclidean quadrant (normalize y to its associate with c > 0, d >= 0, then floor): f in [0, 1)^2 again, same failure.

All are unique, cheap (one floor or truncating division per part), and match the corresponding integer division on the real axis. None meets N(r) < N(y). Truncation alone is unit- and conjugation-equivariant because trunc is odd.

## 4. Nearest (minimal-norm) conventions

The square lattice's Voronoi cell is the square of side 1, so rounding each part of z to the nearest integer gives exactly the minimal-norm remainder, with N(r) <= N(y)/2 (equality only at corners). The minimum is unique except on cell boundaries, which are the ties of section 2: two minimizers on an edge, four at a corner. So no choice makes the minimal-norm remainder unique without a tie rule.

| Tie rule (each part) | f lies in | Odd? | Unit/conj equivariant | r a function of x mod y |
|---|---|---|---|---|
| Ties up, q = floor(z + (1+i)/2) | [-1/2, 1/2)^2 | no | fails at ties | yes |
| Ties down, q = ceil(z - (1+i)/2) | (-1/2, 1/2]^2 | no | fails at ties | yes |
| Ties to even | depends on q | yes | yes, always | no, at ties |
| Ties away from zero | depends on sign of z | yes | yes, always | no, at ties |
| Ties toward zero | depends on sign of z | yes | yes, always | no, at ties |

Equivariance here means q(ux, y) = u*q(x, y), q(x, uy) = q(x, y)/u for units u, and q(conj x, conj y) = conj q(x, y). Multiplying by i maps (s, t) to (-t, s), and conjugation maps (s, t) to (s, -t), so equivariance holds exactly when the part-rounding rule is odd. With an odd rule r(x, uy) = r(x, y), so r does not depend on which associate of y is used, but r is not a function of the residue class at ties (section 1 argument, in two dimensions: at y = 1 + i, x = 1 the four candidates are swapped by units, and x is congruent to ux for every unit u).

All five are unique and give N(r) <= N(y)/2. On the real axis (x, y in Z) the imaginary part of z is 0 and every rule reduces to rounded integer division R with the same tie rule; none agrees with T, F or E. That disagreement is unavoidable for any convention with N(r) <= N(y)/2.

Integer-only cost: compute A, B, N, then per part one floor division A = q0*N + rho with 0 <= rho < N, then a correction. Ties up: q = q0 + (2*rho >= N), written rho >= N - rho to avoid overflow. Ties down: rho > N - rho. Ties to even: add 1 if rho > N - rho, or if rho = N - rho and q0 is odd. Away from zero: on a tie add 1 when q0 >= 0. Toward zero: on a tie add 1 when q0 < 0. The difference between rules is one or two comparisons; the floor division dominates. Ties up can also be one division of 2A + N by 2N, at the cost of one bit of headroom.

## 5. Systems

| System | Function | Quotient rule | Ties | Source status |
|---|---|---|---|---|
| FLINT | fmpzi_divrem | each part of x/y rounded to nearest | up; docs state "N(r) <= N(y)/2, and the pair is uniquely determined" | fetched |
| FLINT | fmpzi_divrem_approx | from a floating-point approximation of x/y | "implementation-defined"; only N(r) < N(y) | fetched |
| PARI/GP | round(x/y), nfeltdivrem, nfeltdiveuc | nearest per coordinate in the integral basis (1, i) | up (round(1/2 + i/2) = 1 + i) | mailing list fetched; manual not verified online |
| SymPy | ZZ_I, GaussianInteger.__divmod__ | qx = (2A + N) // (2N), qy likewise | up | source read |
| Lean Mathlib | GaussianInt `/`, `%` | round of each part of x*conj(y)/N(y) | Mathlib's round is floor(t + 1/2), so up (rule not on the fetched page) | fetched; lemma norm_mod_lt gives N(r) < N(y) |
| malachite (Rust) | GaussianInteger div_rem | floor(x*conj(y)/N(y) + (1+i)/2) per part | up; documents N(r) <= N(y)/2 | fetched |
| Wikipedia | article text | m, n with -1/2 < x - m <= 1/2 | down; notes the half-open interval "is required for uniqueness" | fetched |
| Sage | QuadraticField(-1).ring_of_integers() | not a EuclideanDomain by default; trac 23971 open at the time of the thread | none | fetched (old thread) |
| Mathematica | Quotient, Mod | documented as Floor[m/n] for integers; a complex example is shown without stated rule | not stated | partly fetched; complex behaviour not verified |
| Maple | GaussInt[GIquo], GIrem | not found | not found | not verified online |
| Magma | orders of Q(i) | no Euclidean quotient found in the fetched handbook page | not found | not verified |

FLINT's gcd routines (fmpzi_gcd_euclidean with fmpzi_divrem, an improved version with approximate quotients, Weilert's (1+i)-ary binary gcd, and a shortest-vector version) only need some remainder of smaller norm, which is why the approximate quotient is allowed to be implementation-defined there. Ties up is the de facto standard: FLINT, PARI, SymPy, Mathlib and malachite all use it.

## 6. Fundamental-domain conventions

Choosing a unique q is the same as choosing a fundamental domain F of the lattice Z[i] in C (one representative per class of z mod Z[i]) and taking q so that z - q lies in F. Then:

- q is unique by construction, and there is no tie case: boundary points simply belong to F or not.
- N(r) < N(y) holds exactly when F lies in the open unit disk, and N(r) <= k*N(y) when F lies in the disk of radius sqrt(k). F has area 1, so the best possible is the square, k = 1/2.
- r is a function of (x mod y, y).
- No fundamental domain is invariant under z -> -z, z -> iz or conjugation: the points 1/2 and -1/2 are congruent, as are 1/2 + i/2 and its three rotations, and i/2 and -i/2. So no fundamental-domain convention is unit- or conjugation-equivariant. This is the two-dimensional form of the trade-off in section 1.

Domains that appear in practice or are easy to state:

- [-1/2, 1/2)^2: ties up. Bound 1/2.
- (-1/2, 1/2]^2: ties down (Wikipedia). Bound 1/2.
- [0, 1)^2: floor. Fails (bound approaches 2).
- Axis-compatible variant: [-1/2, 1/2)^2 with the slice Im = 0 replaced by [0, 1) x {0}. It lies in the open unit disk, so N(r) < N(y) with bound approaching 1, and for real x, y it is exactly floored integer division (Euclidean if y is first normalized to be positive). The cost is a branch on B being divisible by N, which is the special case the author wants to avoid.

Making r depend only on the ideal (y), as Boute's E does for Z, needs one more step: replace y by its canonical associate y' = u*y (say c > 0, d >= 0), divide with any fixed F, and return q = q'*u. The result r is a function of (x mod (y)) alone; q still changes with the associate, as in E. Cost: a few negations and a swap of halves.

## 7. Other Euclidean functions and tie-avoiding treatments

- Hurwitz's complex continued fractions use the nearest Gaussian integer with a half-open square, the same choice as section 6 (Hurwitz 1887; not verified online).
- The minimal Euclidean function of Z[i] (Motzkin 1949; Lenstra, who showed it counts digits in a (1+i)-ary expansion) is smaller than the norm. Graves (2025) gives the first division algorithm relative to it: start from the rounded ("Gauss") quotient and, when needed, replace r by r minus a unit multiple of y. It does not remove ties; it inherits whatever rule the starting rounding uses.
- Weilert's (1+i)-ary gcd avoids division with remainder altogether, which sidesteps the quotient question for gcd but not for a div/mod API.
- Lemmermeyer's survey of Euclidean number fields and Kaltofen and Rolletschek's quadratic-field gcd cover general theory; neither proposes a tie-free canonical quotient for Z[i] (not verified online).

No source found offers a convention that is unique, has N(r) < N(y), and treats all units and conjugation symmetrically; section 6 shows none can exist.

## 8. Options for our type

Requirements: integer-only, unique quotient, N(r) < N(y). Every convention below meets all three; floor, truncation, ceiling and the Euclidean quadrant are excluded by section 3. The real decision is which property to drop at the tie points (only divisors divisible by 1 + i have any).

1. Nearest, ties up: q = floor(z + (1+i)/2) per part, f in [-1/2, 1/2)^2. Bound N(y)/2, r a function of x mod y, written as one floor per part with no tie branch, matches FLINT, PARI, SymPy, Mathlib and malachite. It loses unit and conjugation symmetry at ties only. Recommended if the author's objection to "nearest" is the explicit special case: this form has none.
2. Same as 1, after normalizing y to its canonical associate. Adds Boute's property that r depends only on x and the ideal (y). Cost: a unit multiply on the way in and out.
3. Nearest, ties to even or away from zero. Bound N(y)/2 and full equivariance under units and conjugation, but r is not a function of x mod y at ties, and the rule needs an explicit tie comparison. Choose this if symmetry matters more than canonical remainders.
4. Nearest, ties down (Wikipedia's normalization). Same properties as 1; ranked below it only because it disagrees with every library surveyed.
5. Axis-compatible fundamental domain (section 6). The only option that agrees with ordinary floored or Euclidean integer division on the real axis, with N(r) < N(y) but a bound near N(y) rather than N(y)/2, and an explicit special case. Worth it only if real-axis agreement with Z division is a hard requirement.

## Sources

- Boute, R. T., "The Euclidean definition of the functions div and mod", ACM TOPLAS 14(2) (1992) 127-144, [doi:10.1145/128861.128862](https://doi.org/10.1145/128861.128862) (summary via Leijen).
- Leijen, D., "Division and Modulus for Computer Scientists", University of Utrecht, 2001, [publication page](https://www.microsoft.com/en-us/research/publication/division-and-modulus-for-computer-scientists/), [PDF](https://www.microsoft.com/en-us/research/wp-content/uploads/2016/02/divmodnote.pdf).
- Knuth, D. E., The Art of Computer Programming, Vol. 1, section 1.2.4; Graham, Knuth, Patashnik, Concrete Mathematics, section 3.4 (not verified online).
- Common Lisp HyperSpec, floor, ceiling, truncate, round, [page](http://www.lispworks.com/documentation/HyperSpec/Body/f_floorc.htm) (not verified online).
- FLINT, fmpzi module, [docs](https://flintlib.org/doc/fmpzi/index.html).
- Belabas, K., pari-users, 27 May 2025, [message](https://pari.math.u-bordeaux.fr/archives/pari-users-2505/msg00023.html); 27 Nov 2025, [message](https://pari.math.u-bordeaux.fr/archives/pari-users-2511/msg00054.html).
- PARI/GP manual, nfeltdivrem and nfeltdiveuc, [general number fields](https://pari.math.u-bordeaux.fr/dochtml/html/General_number_fields.html) (not verified online).
- SymPy, sympy/polys/domains/gaussiandomains.py, [GitHub](https://github.com/sympy/sympy/blob/master/sympy/polys/domains/gaussiandomains.py) (read from an installed copy of the file).
- Lean Mathlib, Mathlib.NumberTheory.Zsqrtd.GaussianInt, [docs](https://leanprover-community.github.io/mathlib4_docs/Mathlib/NumberTheory/Zsqrtd/GaussianInt.html).
- malachite-nz 0.13.0, gaussian_integer div_rem, [source](https://docs.rs/malachite-nz/0.13.0/src/malachite_nz/gaussian_integer/arithmetic/div_rem.rs.html).
- Wikipedia, "Gaussian integer", Euclidean division section, [article](https://en.wikipedia.org/wiki/Gaussian_integer).
- Ask Sage, "Gaussians as Euclidean domain", [thread](https://ask.sagemath.org/question/44271/gaussians-as-euclidean-domain/).
- Wolfram Language, Quotient, [reference](https://reference.wolfram.com/mathematica/ref/Quotient.html).
- Graves, H., "A Division Algorithm for the Gaussian Integers' Minimal Euclidean Function", 2025, [arXiv:2502.21136](https://arxiv.org/abs/2502.21136).
- Weilert, A., "(1+i)-ary GCD computation in Z[i] as an analogue to the binary GCD algorithm", J. Symbolic Comput. 30 (2000) 605-617 (cited by FLINT; not verified online).
- Motzkin, T., "The Euclidean algorithm", Bull. AMS 55 (1949) 1142-1146; Lenstra, H. W., "Euclidean number fields", Math. Intelligencer 2 (1979-80); Lemmermeyer, F., "The Euclidean algorithm in algebraic number fields", Expo. Math. 13 (1995) 385-416; Kaltofen, E. and Rolletschek, H., "Computing greatest common divisors and factorizations in quadratic number fields", Math. Comp. 53 (1989) 697-720 (all not verified online).
- Hurwitz, A., "Über die Entwicklung complexer Grössen in Kettenbrüche", Acta Math. 11 (1887) 187-200 (not verified online).
