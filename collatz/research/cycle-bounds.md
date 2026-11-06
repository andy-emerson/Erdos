# Lower bounds on cycle length, in exact arithmetic (6 Nov)

This note carries out the plan of cycle-bounds-prior-art.md, Section 6, for the interval step only (no averaged refinement, no circuit or Baker step). Part A proves lower bounds on the number of odd steps of a non-trivial cycle of Barina's 7x ± 1 and of the 5+2i map, with no floating point in any step that decides the answer. Part B compares, for the 5+2i map and other Gaussian maps, the bounds given by the size condition alone, the angle condition alone and both together, and explains the exponents. Notation follows catalog M2 and M9: n odd steps, V divisions, ε the unit of the cycle, r the bound on |d_j/(α z_j)|. No paper or catalog entry was edited and nothing was committed.

## 1. Method and files

1. Scripts in collatz/research/: certarith.py (certified constants and the smallest-denominator routine), cyclebound7.py (Part A for 7x ± 1), cyclebound_gauss.py (Part A for 5+2i and Part B), cyclebound_thresholds.py (verification bounds needed for larger bounds, and the Diophantine constants of Section 5.4).
2. Logs in collatz/research/logs/: cyclebound7.txt, cyclebound_gauss_maps.txt, cyclebound_gauss_scale.txt, cyclebound_gauss_counts.txt, cyclebound_thresholds.txt. Every run takes a few seconds.
3. Arithmetic. A real constant is an integer interval [lo, hi] at scale 2^-320, meaning lo/2^320 ≤ value ≤ hi/2^320. Logarithms are computed as ln(a/b) = k ln 2 + 2 atanh(t) with t = (c - 1)/(c + 1) for the reduced ratio c in [2/3, 4/3), summing the atanh series with separate downward and upward roundings and adding the tail bound t^(2K+1)/((2K+1)(1 - t^2)) to the upper end. π comes from Machin's formula 16 atan(1/5) - 4 atan(1/239), and arg(a + bi) from atan series (reduced by atan(1/2) when the argument exceeds 0.3) with the alternating-series tail bound, placed in the right quadrant. Square roots use integer isqrt with the enclosure isqrt(⌊x⌋) ≤ √x < isqrt(⌊x⌋) + 1. Ratios of intervals become exact Fractions with outward-directed endpoints. Every enclosure was checked against mpmath at 110 digits as a test of the code (not used in any decision).
4. Smallest denominator. For a closed interval [x, y] with rational ends, the fraction of least denominator is found by the continued-fraction recursion: if an integer lies in [x, y] it is the answer; otherwise with f = ⌊x⌋ the answer is f + 1/s where s is the least-denominator fraction in [1/(y - f), 1/(x - f)]. All of it in exact Fractions.
5. Outer and inner enclosures. Each condition is an interval (or a pair of intervals) whose ends are real numbers. A lower bound needs an outer enclosure (a larger interval can only lower the least admissible n), so every bound below is computed with outer enclosures. The same n is then tested against inner enclosures; when it passes, the bound is exactly what the method gives ("attained"), not an artefact of rounding. Attained does not mean a cycle exists.

## 2. Part A: Barina's 7x ± 1

### 2.1 The verified bound

Barina (arXiv:1807.00908, published in Computational Methods in Science and Technology 28(4) (2022), 143 to 147) defines f(n) = 7n + 1 if n ≡ 1 (mod 4), 7n - 1 if n ≡ 3 (mod 4), n/2 if n is even, and writes: "I have computationally verified the convergence of the 7x ± 1 problem for all numbers up to 10^15." The convergence statement is for positive integers (every positive orbit reaches the cycle through 1). The paper does not say how the check was done (whether only odd n, or a stopping-time criterion). For the negative integers it says that "every progression in 7x ± 1 on negative numbers corresponds to negated progression on positive numbers", and that the extended map converges to the cycle through -1. The verification itself is of the positives; the negatives follow from the symmetry f(-x) = -f(x) (catalog U8, A14, in Lean as `IntCollatz.B7_commutes`), which holds because x ≡ 1 (mod 4) exactly when -x ≡ 3 (mod 4).

Consequence for cycles. f maps positive integers to positive integers and negatives to negatives, so a cycle other than {0} lies in the positives or in the negatives, and a negative cycle is the negative of a positive one. A non-trivial cycle (one other than {1, 8, 4, 2}, its negative and {0}) has no element x with 1 ≤ |x| ≤ 10^15: such an element would reach ±1, and an element of a cycle reaches only elements of that cycle. Since 10^15 is even, every odd element satisfies |x| ≥ X := 10^15 + 1. This uses only that every positive integer up to 10^15 reaches 1, which is what Barina states.

### 2.2 The inequality

Let x_0, ..., x_{n-1} be the odd elements of the cycle in order, x_n = x_0, with x_{j+1} 2^(v_j) = 7x_j + d_j, d_j = ±1, v_j ≥ 2, V = Σ v_j. Multiplying x_{j+1}/x_j over the cycle gives 1, so

  2^V = Π (7 + d_j/x_j).

All factors have the same sign as 7 (since |d_j/x_j| < 1), and 7 - 1/X ≤ 7 + d_j/x_j ≤ 7 + 1/X. Hence (7 - 1/X)^n ≤ 2^V ≤ (7 + 1/X)^n, that is,

  V/n ∈ I(X) = [log2(7 - 1/X), log2(7 + 1/X)].

The interval does not depend on n, so n is at least the least denominator of a fraction in I(X). Both sides of log2 7 count, since the digits have both signs (prior art, Section 3.2); I(X) is not symmetric about log2 7 (its lower half is longer by a second-order term), and the computation uses the exact ends.

### 2.3 Result

With X = 10^15 + 1 the outer enclosure of I(X) has width 4.12199·10^-16 and its least-denominator fraction is

  V/n = 78,199,576 / 27,855,251,

which also lies in the inner enclosure (log cyclebound7.txt). The certified continued fraction of log2 7 (45 partial quotients common to both ends of a 320-bit enclosure) is [2; 1, 4, 5, 4, 5, 4, 1, 29, 1, 4, 8, 1, 1, 2, 1, 31, 10, 1, 2, 2, 6, 2, 3, 1, 1, 197, ...], and 78,199,576/27,855,251 is its convergent p_15/q_15, lying above log2 7, at 0.598 of the way across I(X). For n = 27,855,251 the only admissible V is 78,199,576.

Theorem A1. A cycle of Barina's 7x ± 1 other than {0}, {1, 8, 4, 2} and {-1, -8, -4, -2} has at least 27,855,251 odd elements and at least 78,199,576 even ones, so at least 106,054,827 elements (steps of f).

Proof. Section 2.1 gives |x_j| ≥ 10^15 + 1 for every odd element; Section 2.2 gives V/n ∈ I(X); the exact computation shows that no fraction with denominator below 27,855,251 lies in an interval containing I(X). For the second count, V ≥ n log2(7 - 1/X) ≥ 27,855,251 · log2(7 - 1/X), and the least integer V compatible with n ≥ 27,855,251 is 78,199,576 (the ratio V/n is pinned to I(X), and the exact check gives ⌈27,855,251 · min I(X)⌉ = 78,199,576). ∎

The floating-point value of the prior-art note is confirmed. Trusted base: Barina's computation and the correctness of his statement as quoted; the certified arithmetic is in the repository and uses only Python integers and Fractions.

### 2.4 Admissible lengths beyond the bound

If V/n and V'/n' lie in I(X), so does (V + V')/(n + n') (I(X) is an interval), so the admissible n form an additive semigroup containing every multiple of 27,855,251. The reduced fractions in the outer I(X) with denominator at most 10^9 number 125 (Stern-Brocot enumeration in cyclebound7.py); below 3·10^8 they are n = 27,855,251, 159,237,663, 187,092,914, 214,948,165, 230,735,851, 242,803,416, 258,591,102, 270,658,667, 286,446,353, 298,513,918. Together with the multiples of 27,855,251 these are the only lengths below 3·10^8 that the interval step cannot exclude.

### 2.5 Verification needed for larger bounds

For a fraction V/n, the largest X with V/n ∈ I(X) is X*(V/n) = 1/|2^(V/n) - 7|. The bound at X is the least n with X*(V/n) ≥ X for some V, so it jumps at the record values of X* (in order of n), which are among the convergents and intermediate fractions of log2 7. Thresholds below are mpmath values at 80 digits (planning numbers); the bound at the X listed in the last column was checked exactly with the routine of Section 2.3.

| bound n | holds for X up to | exact check |
|---|---|---|
| 27,855,251 | 5.0869·10^15 | 27,855,251 at 5.086·10^15 |
| 465,645,424 | 5.6340·10^15 | 465,645,424 at 5.087·10^15 |
| 604,921,679 | 1.0945·10^16 | at 10^16 |
| 855,618,938 | 1.4291·10^17 | at 10^17 |
| 883,474,189 | 1.7282·10^18 | at 10^18 and 1.728·10^18 |
| 5,328,700,385 | 2.2124·10^18 | at 1.729·10^18 |
| 8,862,597,141 | 2.4372·10^19 | at 10^19 |
| 66,455,550,932 | 6.2478·10^21 | at 10^21 and 2^71 |

Two points differ from the prior-art plan (Section 6.1, item 5). First, the next jump is not at the next convergent 883,474,189 (which needs X ≈ 1.4·10^17) but at the intermediate fraction (p_14 + 16 p_15)/(q_14 + 16 q_15) = 1,307,231,973/465,645,424, which enters I(X) at X ≈ 5.087·10^15: since a_16 = 31, the intermediate fractions with index at least 16 approximate as well as needed. So verification of 7x ± 1 to 5.1·10^15, five times Barina's bound, would raise the bound from 27.9 million to 465.6 million odd steps, and to 10^16 would give 604.9 million. Second, Barina's 7x ± 1 bound of 10^15 sits well inside the plateau (27,855,251 holds for all X from 1.17·10^14 to 5.09·10^15). For comparison, a verification of 7x ± 1 to 2^71 (the 3x+1 record) would give 66,455,550,932.

The bound grows like X^(1/2) (Section 5.3), with jumps of a factor up to the next partial quotient. The interval step saturates at n ≈ (7 ln 2/2) X ≈ 2.4X, where I(X) has length 1/n and every n is admissible.

## 3. Part A: the 5+2i map

### 3.1 The verified bound and what it says about cycle elements

The map (catalog H1) is S(z) = (αz + d(z))/π^v(z) on odd z, with α = 5 + 2i, π = 1 + i, π^4 | αz + d(z), digits D = {±1, ±i, ±(2+i), ±(1-2i)}, so |d| ≤ √5. It commutes with the units (H4). H8 and H9: `gcz` classified every z with N(z) ≤ 10^7; `gstop` checked that every odd z with a > 0, b ≥ 0 and 10^7 < N(z) ≤ 3·10^9 has an iterate of smaller norm; and the drop-below-start lemma (Lean, `H_sound`) turns this into: every nonzero z with N(z) ≤ 3·10^9 reaches ±1 or ±i.

A non-trivial cycle up to units is a list of odd z_0, ..., z_{n-1} with S(z_j) = z_{j+1} and S(z_{n-1}) = ε z_0 for a unit ε, containing no unit. If some element (odd or even) of the cycle on Z[i] generated by z_0 had norm at most 3·10^9, it would reach a unit, and since an element of a cycle reaches only elements of the cycle, a unit would lie on the cycle; as the map commutes with the units, the cycle up to units would then be the trivial one. So every element has N(z) ≥ B + 1 with B = 3·10^9, and for every odd element

  |w_j| = |d_j|/(|α| |z_j|) ≤ r,  r^2 = 5/(29(B + 1)),  r = 7.58098·10^-6.

The literal cycle on Z[i] has n·ord(ε) odd steps, so a bound on n bounds it too.

### 3.2 The cycle equation and the two conditions

From z_{j+1} π^(v_j) = α z_j + d_j (with z_n = ε z_0), multiplying z_{j+1}/z_j over the cycle,

  α^n Π (1 + w_j) = ε π^V.

Write P = Π(1 + w_j) and take moduli and arguments. Each factor lies in the closed disc of radius r about 1.

Size. |P| = 2^(V/2)/29^(n/2), and ln(1 - r) ≤ ln|1 + w_j| ≤ ln(1 + r), so

  n ln(1 - r) ≤ (V ln 2 - n ln 29)/2 ≤ n ln(1 + r),

that is, V/n ∈ [θ_1 + 2 ln(1 - r)/ln 2, θ_1 + 2 ln(1 + r)/ln 2] with θ_1 = log2 29. For the outer enclosure, ln(1 + r) ≤ r and -ln(1 - r) ≤ r/(1 - r); for the inner one, ln(1 + r) ≥ r - r^2/2 and -ln(1 - r) ≥ r.

Angle. The disc of radius r about 1 is seen from 0 under the half-angle arcsin r, so |arg(1 + w_j)| ≤ arcsin r and the sum φ of the principal arguments satisfies |φ| ≤ n arcsin r. Since arg π = π/4 and ε = i^c, φ = Vπ/4 + cπ/2 - n arg α + 2πk, and with W = V + 2(c + 4k),

  |Wπ/4 - n arg α| ≤ n arcsin r,  W ≡ V (mod 2),

that is, W/n ∈ [β - 4 arcsin(r)/π, β + 4 arcsin(r)/π] with β = 4 arg(α)/π. Outer: arcsin r ≤ r/(1 - r^2) (all Taylor coefficients of arcsin are at most 1); inner: arcsin r ≥ r. For a map on Z[i] that does not commute with the units, a non-trivial cycle is a literal cycle, ε = 1, and the condition becomes W ≡ V (mod 8).

Both. The box condition is the conjunction: integers V and W ≡ V (mod m), m = 2 for ε free and 8 for ε = 1, with V in n times the size interval and W in n times the angle interval.

Disc. A sharper joint condition: |log(1 + w)| ≤ Σ r^k/k = -ln(1 - r) ≤ r/(1 - r) = ρ for |w| ≤ r, so the complex number Λ = Σ log(1 + w_j), whose real part is (V ln 2 - n ln 29)/2 and whose imaginary part is Wπ/4 - n arg α, satisfies |Λ| ≤ nρ. This is the condition used in the prior-art note. Every n passing the disc passes the box, so the disc bound is at least the box bound.

### 3.3 Result

The bounds are computed with the outer enclosures: the single conditions by the least-denominator routine (each is a fixed interval for V/n or W/n), the joint ones by scanning n = 1, 2, ... in integer arithmetic (the size and angle windows for n are [⌈n·lo⌉, ⌊n·hi⌋] with dyadic outer ends at 2^-200; the disc is tested exactly on each box survivor with outer Fractions). For the 5+2i map at B = 3·10^9 (log cyclebound_gauss_maps.txt): size alone gives n ≥ 169 (V = 821), angle alone n ≥ 161 (W = 78), both n ≥ 1901 (V = 9235, W = 921), disc n ≥ 1901. All four are attained.

Theorem A2. A non-trivial cycle up to units of the 5+2i map has at least 1901 odd steps and at least 9235 divisions by 1 + i, so at least 11,136 steps of T.

Proof. Section 3.1 gives N(z_j) ≥ 3·10^9 + 1 and |w_j| ≤ r. Section 3.2 shows that (n, V, W) satisfies the size and the angle condition with W ≡ V (mod 2). The exact scan shows that no n ≤ 1900 has integers V and W of the same parity in the outer windows, so n ≥ 1901. For V: the size condition gives V ≥ n(θ_1 + 2 ln(1 - r)/ln 2), and for n ≥ 1901 this lower end is at least ⌈1901 · (outer lower end)⌉ = 9235. ∎

Trusted base: as for H8 (the runs, the C programs, 128-bit arithmetic, the compiler, the rule file); the bound itself adds only the certified arithmetic. The value 1901 of the prior-art note, found there in double precision with the disc condition, is confirmed, and the box condition already gives it.

### 3.4 Verification needed for larger bounds

For each n let r*(n) be the least r at which n passes both conditions, to first order: the minimum over V and W ≡ V (mod 2) of max(|V - nθ_1| ln 2/(2n), |W - nβ| π/(4n)). The bound at B is the least n with r*(n) ≤ r(B), so it equals n for B between the thresholds B*(n') = 5/(29 r*(n')^2) of the previous and of its own record. Double precision, n ≤ 2·10^7 (log cyclebound_thresholds.txt), with exact checks at B = 3, 4, 5·10^9 (1901), 10^10 (2190), 3·10^10 (4091), 10^11 (6281), 10^12 and 10^13 (8696), 10^14 (58,457), 10^15 (222,942):

| bound n | for B up to about |
|---|---|
| 1676 | 2.27·10^9 |
| 1901 | 7.55·10^9 |
| 2190 | 1.47·10^10 |
| 4091 | 5.03·10^10 |
| 6281 | 1.24·10^11 |
| 8696 | 1.32·10^13 |
| 58,457 | 1.99·10^14 |
| 67,153 | 9.72·10^14 |
| 298,791 | 1.49·10^16 |
| 673,431 | 2.94·10^18 |
| 4,049,282 | 1.56·10^20 |

So the current verification has room: B = 7.5·10^9 (2.5 times the present run, about 25 minutes more on one core at the H8 rate) would still give 1901, and B = 7.6·10^9 gives 2190; B = 1.5·10^10 gives 4091 and B = 1.3·10^11 gives 8696. Then a long plateau (8696 holds to 1.3·10^13), because 8696 is an unusually good joint approximation (r*(8696) = 1.14·10^-7). The lengths in the table are the records of the prior-art note (2190, 4091, 8696, 67153, 298791, 673431), as they should be. The interval step saturates at n_sat = max(ln 2/(4r), 2π/(8r)) ≈ 1.04·10^5 at B = 3·10^9: from there on both windows are long enough to contain admissible V and W of every parity, so the basic method can never give more than about 10^5 at this B, and never more than order B^(1/2) at any B.

## 4. Part B: three bounds for several Gaussian maps

### 4.1 The maps

All are Collatz maps (or conjectured so) with their verification bounds from the catalog. The commuting maps and the cone map have ε free (m = 2); the five residue-class maps at (1+i)^2 with digits D = {1, i} do not commute with the units and act on Z[i] itself, so ε = 1 (m = 8).

1. 5+2i at (1+i)^4: B = 3·10^9, |d|^2 ≤ 5 (H8, H9).
2. Single-cycle commuting maps at (1+i)^6 (G41): 7+4i, 8+3i, 9+2i, 10+i, 10+3i, 11+2i, B = 10^6, |d|^2 ≤ 25 (every digit set at this modulus contains 3 ± 4i). Conjugates and unit multiples give the same bounds when ε is free (the conditions depend on β only modulo 2 and up to sign).
3. Single-cycle commuting maps at (1+i)^7 (G45): -11+4i, -12+3i, -11+8i, -12+7i (the representatives checked to 10^9), B = 10^9, |d|^2 ≤ 49 (from the rule files k7_*.rule).
4. The cone map 2+i at (1+i)^2 on Z[i]/U (fundamental-domain.md, Section 7.3): digits -1, -i, |d| = 1, every class of norm at most 10^7 reaches [1], so B = 10^7. On the quotient, S(z) = rep_C((αz + d)/π^v) differs from the residue-class step by a unit, so the product of the units goes into ε and the cycle equation is the same with ε free.
5. The residue-class maps E_2 at (1+i)^2 (G4 to G6): multipliers 2-i, -1-2i, -2-i, 1+2i, -2+i, D = {1, i}, all-quadrant check to 10^7 (V2), B = 10^7, ε = 1.

### 4.2 The three bounds at the actual verification bounds

Exact values (log cyclebound_gauss_maps.txt). Every value is attained (passes the inner enclosures). The last column is the bound from both conditions divided by the larger single bound.

| map | B | size alone | angle alone | both (box) | disc | ratio |
|---|---|---|---|---|---|---|
| 5+2i | 3·10^9 | 169 | 161 | 1901 | 1901 | 11.2 |
| 7+4i | 10^6 | 42 | 53 | 251 | 251 | 4.7 |
| 8+3i | 10^6 | 21 | 35 | 105 | 105 | 3.0 |
| 9+2i | 10^6 | 22 | 18 | 61 | 61 | 2.8 |
| 10+i | 10^6 | 35 | 55 | 79 | 79 | 1.4 |
| 10+3i | 10^6 | 13 | 35 | 116 | 116 | 3.3 |
| 11+2i | 10^6 | 29 | 35 | 144 | 144 | 4.1 |
| -11+4i | 10^9 | 51 | 286 | 1153 | 1153 | 4.0 |
| -12+3i | 10^9 | 101 | 109 | 1449 | 1760 | 13.3 |
| -11+8i | 10^9 | 207 | 321 | 988 | 988 | 3.1 |
| -12+7i | 10^9 | 211 | 293 | 876 | 876 | 3.0 |
| cone 2+i | 10^7 | 59 | 61 | 388 | 388 | 6.4 |
| E_2, 2-i | 10^7 | 59 | 61 | 813 | 813 | 13.3 |
| E_2, -1-2i | 10^7 | 59 | 61 | 671 | 671 | 11.0 |
| E_2, -2-i | 10^7 | 59 | 61 | 388 | 388 | 6.4 |
| E_2, 1+2i | 10^7 | 59 | 61 | 693 | 693 | 11.4 |
| E_2, -2+i | 10^7 | 59 | 61 | 515 | 537 | 8.4 |

In every case the joint bound exceeds both single bounds. Neither condition is binding; the size bound and the angle bound are of the same order (the angle allowance 4r/π per step is smaller than the size allowance 2r/ln 2, but the two constants are close and the partial quotients decide which is larger), and the joint bound is larger by a factor between 1.4 and 13. The disc adds nothing in 15 of 17 cases. For the E_2 family the single bounds coincide (all five multipliers share N(α) = 5, and their β agree modulo 1 up to sign, which is all the angle condition alone sees), while the joint bounds differ, because with ε = 1 the angle is read modulo 2π rather than π/2; -2-i, whose β differs from that of the cone map 2+i by 4, gives the cone value 388 because the cone's first solution (n, V, W) = (388, 901, 229) happens to satisfy W ≡ V (mod 8).

These bounds apply to the maps as conjectured Collatz maps. For the level 6 maps they are weak because B = 10^6 is small.

### 4.3 As functions of B

Exact bounds for B = 10^3 to 10^20 (log cyclebound_gauss_scale.txt), sample rows for the 5+2i map:

| B | size | angle | both | disc |
|---|---|---|---|---|
| 10^3 | 6 | 2 | 6 | 21 |
| 10^5 | 7 | 31 | 64 | 64 |
| 10^7 | 120 | 64 | 225 | 225 |
| 10^9 | 169 | 161 | 1676 | 1676 |
| 10^11 | 683 | 1224 | 6281 | 6506 |
| 10^13 | 852 | 1707 | 8696 | 41065 |
| 10^15 | 4943 | 6667 | 222,942 | 222,942 |
| 10^17 | 15,681 | 15,041 | 673,431 | 673,431 |
| 10^20 | 67,667 | 382,692 | 4,049,282 | 4,049,282 |

Over the 50 pairs (map, B) in the log (5+2i for B = 10^3 to 10^20, cone 2+i and E_2 2-i for 10^3 to 10^18) the joint bound exceeds both single bounds in 47. The exceptions are all at small B: the 5+2i map at B = 10^3 (joint = size = 6), and the cone map and 2-i at B = 10^5, where all three equal 22 ((2 ± i)^22 is close to a unit times π^51 in both size and angle). From B = 10^6 on, no exception occurs. A least-squares fit of log n against log B over the 5+2i rows from 10^9 to 10^20 gives slopes 0.331 for the joint bound, 0.242 for size alone and 0.246 for angle alone, against the predicted 1/3 and 1/4 (Section 5.3).

## 5. Part B: how the conditions combine

### 5.1 Elementary facts

1. n_both ≥ max(n_size, n_angle) and n_disc ≥ n_both, since each adds a condition.
2. n_both = n_size exactly when the least size-admissible n also passes the angle condition (with the parity coupling), and symmetrically for the angle; otherwise n_both is larger than both.
3. Ceiling. With r' the inner value of r, every n ≥ max(ln 2/(4r'), mπ/(8r')) is admissible: the size window for V then has length at least 1, the angle window for W length at least m, and a closed interval of length m contains every residue class mod m. So the method gives at most order 1/r ≍ B^(1/2) for every Gaussian map, and order X for 7x ± 1.

### 5.2 Counting model

If n θ_1 and n β are equidistributed modulo 1 (jointly), the probability that n passes the size condition is the window length 2n a_s with a_s = 2r/ln 2, the angle condition 2n a_a with a_a = 4r/π, and both (2n a_s)(2n a_a)/m. The expected number of admissible n ≤ M is then a_s M^2, a_a M^2 and 4 a_s a_a M^3/(3m), and the first admissible n is predicted at a_s^(-1/2), a_a^(-1/2) and (3m/(4 a_s a_a))^(1/3). For the 5+2i map at 3·10^9 the predictions are 214, 322 and 1922, against 169, 161 and 1901. The exact counts of admissible n ≤ M (log cyclebound_gauss_counts.txt) follow the model until saturation: for 5+2i, 197, 87 and 4 admissible n ≤ 3000 against 196.9, 86.9 and 3.80; 2189, 967 and 140 for n ≤ 10^4 against 2187.4, 965.2 and 140.8. In this model the chance that the joint bound equals a single one is about the chance that the first single solution passes the other condition, 2 n_single a_other/m ≍ r^(1/2), which is 10^-3 for 5+2i at 3·10^9; that matches the table (equality only at small B).

### 5.3 A metric theorem

Write x = θ_1 = log2 N(α) and y = (β - θ_1)/m with β = 4 arg(α)/π. With W = V + mc, the conditions read |V - nx| ≤ A_1 n r and |(V - nx) + m(c - ny)| ≤ A_2 n r up to factors 1 + O(r), with A_1 = 2/ln 2 and A_2 = 4/π. So there are constants 0 < k_1 ≤ k_2 depending only on m such that max(‖nx‖, ‖ny‖) ≤ k_1 n r implies that n passes both conditions, and passing both implies max(‖nx‖, ‖ny‖) ≤ k_2 n r. The single conditions are ‖nx‖ ≤ A_1 n r and ‖nβ‖ ≤ A_2 n r.

Theorem B1. For Lebesgue-almost every pair (log2 N(α), 4 arg(α)/π), viewed as a pair of real parameters, as r → 0,

  n_size(r) = r^(-1/2 + o(1)),  n_angle(r) = r^(-1/2 + o(1)),  n_both(r) = r^(-2/3 + o(1)).

In particular n_both/max(n_size, n_angle) = r^(-1/6 + o(1)) tends to infinity, and with r ≍ B^(-1/2), n_both = B^(1/3 + o(1)) while each single bound is B^(1/4 + o(1)). For 7x ± 1 the same argument in one dimension gives X^(1/2 + o(1)) for almost every multiplier in place of log2 7.

Proof. The map (θ_1, β) ↦ (x, y) is an invertible affine map, so "almost every" may be read in (x, y). Two facts.

(a) Borel and Cantelli. For δ > 0 and almost every (x, y), max(‖nx‖, ‖ny‖) ≥ n^(-1/2-δ) for all n ≥ n_0(x, y): the set of (x, y) in the unit square with max(‖nx‖, ‖ny‖) < ψ has measure 4ψ^2, and Σ n^(-1-2δ) converges. In one dimension, ‖nx‖ ≥ n^(-1-δ) eventually, for almost every x.

(b) Dirichlet. For every Q ≥ 1 there is 1 ≤ n ≤ Q with max(‖nx‖, ‖ny‖) ≤ 1/⌊Q^(1/2)⌋: cut the unit square into ⌊Q^(1/2)⌋^2 ≤ Q boxes and apply the pigeonhole principle to the Q + 1 points (jx, jy), 0 ≤ j ≤ Q. In one dimension there is n ≤ Q with ‖nx‖ < 1/Q.

Lower bound. If n passes both conditions and n ≥ n_0, then n^(-1/2-δ) ≤ k_2 n r, so n ≥ (k_2 r)^(-1/(3/2+δ)). Each fixed n < n_0 fails once r is small (‖nx‖ > 0 for almost every x). Hence n_both(r) ≥ r^(-2/3+o(1)).

Upper bound. Let η > 0 and Q = r^(-2/3-η). By (b) some n ≤ Q has max(‖nx‖, ‖ny‖) ≤ 2Q^(-1/2), and by (a) such an n satisfies n ≥ (Q^(1/2)/2)^(2/(1+2δ)) = Q^(1-O(δ)). It passes both conditions if 2Q^(-1/2) ≤ k_1 n r, which holds if 2Q^(-1/2) ≤ k_1 r Q^(1-O(δ)), that is, r^(1/3+η/2) ≤ (k_1/2) r^(1-(2/3+η)(1-O(δ))) = (k_1/2) r^(1/3 - η + O(δ)); for δ small against η this holds for all small r. So n_both(r) ≤ r^(-2/3-η) for small r, for every η > 0. The single conditions are the same argument in one dimension with exponents 1 and 1/2. ∎

What Theorem B1 does not give. It says nothing about a specific α: log2 29 and arg(5+2i)/π are transcendental, and nothing like the almost-everywhere exponents is proven for them. Baker's method (Matveev's bound for the three logarithms log(5+2i), log(1+i), log i) gives an effective lower bound |Λ| ≥ n^(-C), which bounds n_both below only by about r^(-1/(1+C)); C was not computed here, but Matveev's constants for three logarithms are astronomically large (Simons 2022 works with an exponent of order 10^18 in a similar three-logarithm problem), so this is useless for exponents. For a specific map the exponents are therefore an observation over a finite range, which Section 5.4 makes quantitative.

### 5.4 Finite-range constants

Over 10 ≤ n ≤ 2·10^7 (double precision, log cyclebound_thresholds.txt) the quantities n^2 r_size*(n), n^2 r_angle*(n) and n^(3/2) r*(n) (r* the joint threshold of Section 3.4) have minima c_size, c_angle, c_both:

| map | c_size | c_angle | c_both |
|---|---|---|---|
| 5+2i | 0.0036 | 0.0221 | 0.0926 |
| cone 2+i | 0.0176 | 0.0513 | 0.1340 |
| 2-i, ε = 1 | 0.0176 | 0.0513 | 0.1340 |
| -12+3i | 0.0014 | 0.0205 | 0.1096 |
| 7+4i | 0.0191 | 0.0172 | 0.0756 |

So within that range n_size ≥ (c_size/r)^(1/2), n_angle ≥ (c_angle/r)^(1/2) and n_both ≥ (c_both/r)^(2/3), as long as the right side stays below 2·10^7 and above 10. The joint constants are of order 0.1 for every map, the single ones vary more (they are set by the best one-dimensional approximation met in the range; for the size condition of 5+2i the minimum is at n = 3,700,209). These are the finite-range forms of the exponents 1/2 and 2/3; for the 5+2i map at 3·10^9 they give only 530 against the actual 1901, because the minimum of n^(3/2) r* is reached at the exceptionally good n = 8696 (8696^(3/2) · 1.142·10^-7 = 0.0926).

### 5.5 Why the cube root

The integer bound grows like X^(1/2) and the Gaussian one like B^(1/3), where X bounds |x| and B bounds N(z) = |z|^2. Both the number of conditions and the passage from norms to absolute values enter, and only together do they give the exponent.

1. The allowance is set by absolute values. For Z the per-step error is |d/x| ≤ 1/X; for Z[i] it is |w_j| ≤ r ≍ |z|^(-1) = N(z)^(-1/2). One condition with allowance ε per step gives n ≍ ε^(-1/2), so the Gaussian size condition alone gives |z|^(1/2) = B^(1/4), the same function of |z| as Z gives of |x|. That is the norm effect alone: in terms of B it halves the exponent.
2. The second condition raises it. Two conditions with allowance ε give n ≍ ε^(-2/3) (Theorem B1), so |z|^(2/3) = B^(1/3). Measured in |z| the Gaussian bound is better than the integer one (2/3 against 1/2).
3. The common form. Let d be the number of real conditions, d = [K : Q] (one for Z, size and angle for an imaginary quadratic field). The allowed region for the error vector of step n has d-dimensional volume of order (n · allowance)^d = n^d/N(z) in both cases: n/X for Z, (nr)^2 ≍ n^2/B for Z[i]. The norm is exactly the product of the per-coordinate allowances, so the expected number of admissible n ≤ M is of order M^(d+1)/B, and the first admissible n is of order B^(1/(d+1)): B^(1/2) for Z, B^(1/3) for Z[i]. The norm is also what a verification pays for (the number of elements below the bound is about 2X in Z and πB in Z[i]), so in terms of work the Gaussian bound is a smaller power.

So the smaller exponent is a property of neither alone. The total allowance volume per step is 1/N(z) in every ring; splitting it over d coordinates, each coordinate gets N^(-1/d), and the gain from needing d coincidences (exponent d/(d+1) in 1/ε) does not make up for the smaller allowance per coordinate (ε = N^(-1/d)), giving 1/(d+1). The argument extends to imaginary quadratic fields with any unit group (the torsion units only change m). It does not extend directly to fields with infinite unit groups, where N(z) > B does not bound each |σ(z)| from below.

## 6. Summary of what is proven

Proven, with certified arithmetic and the cited verifications as the only computational inputs:

1. Theorem A1: a non-trivial cycle of 7x ± 1 has at least 27,855,251 odd and 106,054,827 total elements, from Barina's verification of all positive n up to 10^15 and the symmetry f(-x) = -f(x). The bound is what the interval step gives at that verification bound (attained), and it rises to 465,645,424 once the verification reaches 5.09·10^15.
2. Theorem A2: a non-trivial cycle up to units of the 5+2i map has at least 1901 odd steps and 11,136 steps of T, from H8 and H9 (every nonzero z with N(z) ≤ 3·10^9 reaches a unit). The bound is attained by the method; it rises to 2190 at B ≈ 7.6·10^9 and to 8696 at B ≈ 1.3·10^11.
3. The exact single and joint bounds of Section 4.2 for 16 further Gaussian maps, each conditional only on that map's verification as recorded in the catalog.
4. Section 5.1, the ceiling n ≲ 1/r for the basic method, and Theorem B1 (almost everywhere, not for the specific constants).

Not proven: the exponents 1/4 and 1/3 for the specific maps (observed over B = 10^3 to 10^20 and n ≤ 2·10^7, with the finite-range constants of Section 5.4); the threshold tables of Sections 2.5 and 3.4 are double-precision or 80-digit planning numbers, exact only at the B or X listed. Not attempted: the averaged refinement (prior art, Section 6.1, item 3), circuit structure and Baker bounds.
