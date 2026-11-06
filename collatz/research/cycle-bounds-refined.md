# Refining the method behind the cycle-length bounds (6 Nov)

This note continues cycle-bounds.md (Part A: 27,855,251 odd steps for Barina's 7x ± 1 at X = 10^15, and 1901 odd steps for the 5+2i map at B = 3·10^9). It covers the four items that note's Section 6 left out: the averaged refinement, circuit (Steiner) structure, Baker-type bounds and lattice reduction. Notation is that of cycle-bounds.md: n odd steps, V divisions, d_j digits, ε the unit, r^2 = 5/(29(B + 1)), θ_1 = log2 29, β = 4 arg(5+2i)/π. The verification ranges were not touched, cycle-bounds.md was not edited, no paper or catalog entry was edited, and nothing was committed.

## 1. Files

1. Scripts in collatz/research/: cyclebound_avg.py (certified averaged constants and the averaged bounds), cyclebound_avg_B.py (averaged 5+2i bounds at larger B and the jump points beyond 8696), cyclecircuit.py (Steiner-type 1-cycles: Baker bounds, reduction, exhaustive search), cyclebound_lll.py (LLL candidates for the joint condition, planning only). They import certarith.py and cyclebound_gauss.py unchanged.
2. Logs in collatz/research/logs/: cyclebound_avg.txt (about 6 minutes), cyclebound_avg_B.txt (about 15 minutes), cyclecircuit.txt (seconds), cyclebound_lll.txt (about 2 minutes).

## 2. The averaged inequality

### 2.1 Idea

The basic method bounds each per-step error by its worst value: 1/(7X) per odd step for 7x ± 1 and r per odd step for 5+2i. In the 3x+1 literature, Eliahou's criterion is sharpened by bounding the sum of the errors instead. Halbeisen and Hungerbühler (Theorem 3) replace min C by min C/0.9. Hercher (arXiv:2201.00406, Theorem 27) shows that the sum of 1/x over the odd elements is at most (3/4)K/X_0. The gain comes from the fact that a cycle cannot keep all its odd elements near the floor X: every step with the least valuation multiplies |x| by about 7/4 (by about √29/4 for 5+2i), and the elements after such a step are well above the floor. Below, the best constant of this kind that follows from the step sizes alone is computed and certified for both maps. The same computation is run on 3x+1 as a check.

### 2.2 Heights

7x ± 1. Let x_0, ..., x_{n-1} be the odd elements of a non-trivial cycle, with x_{j+1} = (7x_j + d_j)/2^(v_j), d_j = ±1, v_j ≥ 2, and |x_j| ≥ X = 10^15 + 1 (cycle-bounds.md, Section 2.1; negative cycles are negatives of positive ones, and |x| treats both). Put u_j = |x_j|/X ≥ 1. Then

  (7u_j - 1/X)/2^(v_j) ≤ u_{j+1} ≤ (7u_j + 1/X)/2^(v_j).

5+2i. For the odd elements z_j of a non-trivial cycle up to units, z_{j+1} π^(v_j) = αz_j + d_j (z_n = εz_0, |εz_0| = |z_0|), v_j ≥ 4, |d_j| ≤ √5 and N(z_j) ≥ B + 1 (cycle-bounds.md, Section 3.1). Put u_j = |z_j|/√(B + 1) ≥ 1. Then

  (√29 u_j - √5/√(B+1))/2^(v_j/2) ≤ u_{j+1} ≤ (√29 u_j + √5/√(B+1))/2^(v_j/2).

Even elements need no constraint: between two odd elements they are at least twice (in absolute value, at least √2 times for Z[i]) the next odd one.

### 2.3 The bin graph and the certificate

Cut [1, U) into bins [e_k, e_{k+1}) with e_k ≈ (1 + η)^k (integer numerators over 2^40), and add a top bin [U, ∞). Draw an edge k → j if some u in bin k can step into bin j for some admissible v, with every interval computed with outward integer rounding (so every real step is an edge; extra edges only weaken the result). Give bin k the weight w_k = ⌈2^80/E_k⌉/2^40 ≥ 1/u for u in the bin (E_k = 2^40 e_k).

Lemma 1. Let g be any real function on the bins and λ = max_k (w_k + max_{k→j} g_j - g_k). For every non-trivial cycle, (1/n) Σ_j 1/u_j ≤ λ.

Proof. The bins b_j of u_j form a closed walk b_0 → b_1 → ... → b_n = b_0 in the graph, by the outward rounding. Along each edge, w_{b_j} - λ ≤ g_{b_j} - g_{b_{j+1}}. Summing around the walk, the right side telescopes to 0, so Σ_j 1/u_j ≤ Σ_j w_{b_j} ≤ nλ. ∎

So any potential g gives a valid constant. The script finds a good g by damped value iteration in floating point. It then rounds g to integers at scale 2^40 and recomputes λ exactly in integer arithmetic (a range maximum over integer arrays). Only the last step is part of the proof. Since λ is at least the maximum cycle mean of the graph, the float lower estimate printed beside it shows how close the certificate is.

### 2.4 The averaged conditions

Theorem 1 (7x ± 1). Let c_7 = 798486727137/2^40 = 0.726219. For every non-trivial cycle of Barina's 7x ± 1 map,

  -c_7 n/(7X - 1) ≤ V ln 2 - n ln 7 ≤ c_7 n/(7X),  X = 10^15 + 1.

Proof. 2^V = Π|7 + d_j/x_j| = 7^n Π(1 + d_j/(7x_j)), so V ln 2 - n ln 7 = Σ ln(1 + d_j/(7x_j)). Each term is at most ln(1 + 1/(7|x_j|)) ≤ 1/(7|x_j|). Each term is at least ln(1 - 1/(7|x_j|)) ≥ -1/(7|x_j| - 1) ≥ -(X/(7X - 1))/|x_j|; the last step holds because (7X - 1)|x_j| ≤ X(7|x_j| - 1) is equivalent to |x_j| ≥ X. Finally Σ 1/|x_j| = (1/X) Σ 1/u_j ≤ c_7 n/X by Lemma 1, with the certificate of Section 3. ∎

Theorem 2 (5+2i). Let c_G = 935255468522/2^40 = 0.850610 and B ≥ 3·10^9. Write w_j = d_j/(αz_j), so that α^n Π(1 + w_j) = επ^V. For every non-trivial cycle up to units of the 5+2i map,

  Σ|w_j| ≤ c_G n r,

and therefore:
- size: (V ln 2 - n ln 29)/2 lies in [-c_G n r/(1 - r), c_G n r];
- angle: |Wπ/4 - n arg α| ≤ c_G n r/(1 - r^2), with W ≡ V (mod 2);
- disc: |Λ| ≤ c_G n r/(1 - r).

Proof. |w_j| ≤ (√5/√29)/|z_j| = (√5/√29)(B + 1)^(-1/2)/u_j, so Σ|w_j| ≤ r Σ 1/u_j ≤ c_G n r by Lemma 1. Per factor: ln(1 + |w|) ≤ |w|, -ln(1 - |w|) ≤ |w|/(1 - r), arcsin|w| ≤ |w|/(1 - r^2) and |log(1 + w)| ≤ -ln(1 - |w|), exactly as in cycle-bounds.md, Section 3.2, but summed before bounding.

The certificate was computed at B = 3·10^9. It stays valid for larger B, because the offset √5/√(B+1) in the step bounds only shrinks, so the edges for larger B form a subset of those for 3·10^9. ∎

The digit sizes are taken at their worst (|d| = √5 on every step). The digit is a function of z mod 4, and the class of the next odd element is not determined by the class of the current one, so nothing better follows without tracking residues to higher powers of 2.

## 3. The constants

From logs/cyclebound_avg.txt:

| map | η = 1/512, U = 32 | η = 1/2048, U = 32 | η = 1/4096, U = 64 | float lower estimate |
|---|---|---|---|---|
| 3x+1 (positive, X = 2^71) | 0.743162 | 0.728362 | 0.727271 | 0.727271 |
| 7x ± 1 (X = 10^15 + 1) | 0.734787 | 0.731320 | 0.726219 | 0.726074 |
| 5+2i (B = 3·10^9) | 0.867346 | 0.852589 | 0.850610 | 0.849523 |

The 3x+1 row is the calibration. The same code, with the step u → [3u/2^v, (3u + 1/X)/2^v], v ≥ 1, certifies 0.7273. That is a little below Hercher's 3/4 (his Theorem 27), and it uses only the step sizes, not his residue analysis. So the method reproduces the published averaged constant for 3x+1 and slightly improves it. This is a side remark, not checked against his computer-assisted Corollary 29.

The constants are close to the true value of the continuous problem. The best closed walks found have means of 0.7261 (7x ± 1) and 0.8500 (5+2i), they stay between u = 1 and u ≈ 2 (resp. 1.41), and they keep returning to the floor. No argument that uses only the step sizes can go below about 0.726 and 0.850. The 5+2i constant is larger because its up step, the factor √29/4 ≈ 1.346, is smaller than 7/4.

## 4. Effect on the bounds

### 4.1 7x ± 1

The interval for V/n becomes [log2 7 - c_7/((7X - 1) ln 2), log2 7 + c_7/(7X ln 2)], against [log2(7 - 1/X), log2(7 + 1/X)] before. That amounts to replacing X by X/c_7 = 1.377X. Exact least denominators (logs/cyclebound_avg.txt):

| X | averaged bound | basic bound |
|---|---|---|
| 10^15 + 1 | 27,855,251 | 27,855,251 |
| 3·10^15 | 27,855,251 | 27,855,251 |
| 4·10^15 | 465,645,424 | 27,855,251 |
| 5·10^15 | 521,355,926 | 27,855,251 |
| 10^16 | 660,632,181 | 604,921,679 |

At Barina's 10^15 the bound stays 27,855,251, because the basic bound holds on the plateau up to X = 5.0869·10^15, and a jump at 10^15 would need c ≤ 0.197. What averaging changes is the verification needed. In first order, the X at which a record fraction leaves the admissible interval is multiplied by c_7. The record 27,855,251 holds up to 3.694·10^15 (instead of 5.087·10^15), 465,645,424 up to 4.09·10^15, 604,921,679 up to 7.95·10^15, and 883,474,189 up to 1.26·10^18 (instead of 1.73·10^18). The table entries are exact at the X listed; the thresholds are mpmath planning numbers.

Corollary 1. If the 7x ± 1 verification is extended to 4·10^15, a non-trivial cycle has at least 465,645,424 odd elements. (This is conditional on that verification; the averaged bound at 4·10^15 is exact, from Theorem 1.)

### 4.2 5+2i at the current B

At B = 3·10^9 the averaged conditions give size 169, angle 161, box 1901 (V = 9235, W = 921) and disc 1901. These are the same values as before, so Theorem A2 is unchanged. The next value, 2190, is reached by the averaged disc from B = 4·10^9 and by the averaged box from 5.5·10^9 (tested at 4, 5, 5.5, 6, 7·10^9). Without averaging the box needs 7.55·10^9.

### 4.3 5+2i at B = 1.3·10^11 and the next jump

This answers the coordinator's question (logs/cyclebound_avg_B.txt, exact scans to n = 3·10^5, bisection to 0.1 percent). At B = 1.3·10^11 every variant gives 8696: box and disc, with and without averaging. The bound leaves 8696 at the following B, in each case for 38875:

| condition | 8696 holds up to | next bound |
|---|---|---|
| basic box | about 1.32·10^13 | 38875 |
| basic disc | 6.92·10^12 | 38875 |
| averaged box | 9.56·10^12 | 38875 |
| averaged disc | 5.01·10^12 | 38875 (41065 from about 8·10^12, 49761 at 1.32·10^13) |

So passing 8696 needs B ≈ 5.0·10^12, about 38 times 1.3·10^11. Averaging gains a factor of 2.6 in B over the basic box, and a factor of 1.4 over the basic disc. The disc was already better than the box here without averaging: cycle-bounds.md, Section 4.3, lists 41065 at 10^13 for the disc. This was overlooked in that note's threshold table, which follows the box.

## 5. Circuits: Steiner-type 1-cycles

### 5.1 Definition

A 1-cycle of 7x ± 1 is a cycle whose odd elements, read from a suitable x_0, take n - 1 steps with v = 2 and one and the same digit d, then one step with v = l + 2 ≥ 3 and any digit e. For the 5+2i map it is a cycle up to units with n - 1 steps with v = 4 and one digit d, then one step with v = u ≥ 5, digit e, landing on εz_0. This is the analogue of Steiner's 1-cycle (one circuit: a run of increases, then one descent). The run digit must be constant, because the closed form of a run needs it. The descent digit is free.

A cycle with no step of larger valuation is impossible. For 7x ± 1, |x'| ≥ (7|x| - 1)/4 > |x| for |x| > 1/3 when v = 2. For 5+2i, |z'| ≥ (√29|z| - √5)/4 > |z| for |z| > 1.61 when v = 4, and the only odd z with |z| ≤ 1.61 are the units, which have v = 5.

### 5.2 The cycle equation

7x ± 1. Put a_j = 3x_j + d. For a step with v = 2 and digit d, a_{j+1} = (7/4)a_j exactly: 3(7x + d)/4 + d = 7(3x + d)/4. After n - 1 such steps a_{n-1} = (7/4)^(n-1) a_0, so 4^(n-1) divides a_0; write a_0 = 4^(n-1)h and a_{n-1} = 7^(n-1)h. Then h ≠ 0, since 3x_0 + d = 0 has no integer solution. The descent gives 3·2^(l+2)x_0 = 3(7x_{n-1} + e) = 7(7^(n-1)h - d) + 3e. Substituting 3x_0 = 4^(n-1)h - d and V = 2n + l,

  h(2^V - 7^n) = d(2^(l+2) - 7) + 3e.  (1)

5+2i. Put a_j = (α - π^4)z_j + d, with α - π^4 = 9 + 2i. For a step with v = 4 and digit d, a_{j+1} = (α/π^4)a_j. So a_0 = π^(4(n-1))h, a_{n-1} = α^(n-1)h, and h ≠ 0 since N(9 + 2i) = 85 does not divide N(d) ∈ {1, 5}. For the descent, note that the element after it is εz_0, with a-value εa_0 (the map commutes with the units, so the run digit of εz_0 is εd). This gives, with V = 4n + u - 4,

  h(επ^V - α^n) = dεπ^u - dα + (α - π^4)e.  (2)

### 5.3 The linear forms are exponentially small

7x ± 1. The right side of (1) is nonzero (it would need 2^(l+2) ∈ {4, 10}) and at most 2^(l+2) in absolute value. Since |h| ≥ 1, |2^V - 7^n| ≤ 2^(V-2n+2). For n ≥ 2, t = 4^(1-n) ≤ 1/4 and |V ln 2 - n ln 7| ≤ t/(1 - t) ≤ (16/3)4^(-n).

The divisibility version is sharper but not needed. D = 2^V - 7^n is odd and divides 4(2^l - 1) when e = d, so |D| ≤ 2^l - 1, which is Steiner's inequality transposed.

5+2i. The right side Y of (2) has |Y| ≤ √5·2^(u/2) + √5·√29 + √85·√5. Since h ≠ 0, |επ^V - α^n| ≤ |Y|, and |επ^V - α^n| ≥ |2^(V/2) - 29^(n/2)|. Dividing by 2^(V/2) with V ≥ 4n + 1 gives |1 - 29^(n/2)/2^(V/2)| ≤ 32.04·4^(-n). Hence |V ln 2 - n ln 29| ≤ 74·4^(-n) for n ≥ 4. Only the size is used, so this is a form in two real logarithms; the angle is not needed.

### 5.4 Theorem

Theorem 3. Barina's 7x ± 1 map has no 1-cycle other than {1, 8, 4, 2} and its negative. The 5+2i map has no 1-cycle up to units other than the trivial one through the units (S(1) = -1, n = 1, V = 5). Neither statement uses a verification.

Proof. By Section 5.3, |V - nθ| < C·4^(-n) with θ = log2 7, C = 16/(3 ln 2) (n ≥ 2), or θ = log2 29, C = 74/ln 2 (n ≥ 4).

Upper bound. Laurent's bound (Section 6) gives n ≤ 22,614 for 7x ± 1 and n ≤ 39,134 for 5+2i. Matveev's bound alone gives n ≤ 1.93·10^10 and n ≤ 3.48·10^10.

Reduction. The continued fractions of log2 7 and log2 29 are certified to 45 partial quotients (denominators up to 4·10^25). For 0 < n < q_{k+1}, |nθ - V| ≥ |q_kθ - p_k| (best approximation). Take N0 to be the Matveev bound. For 7x ± 1, N0 < q_19 = 28,354,739,801 and |q_18θ - p_18| ≥ 3.04·10^-11, so C·4^(-n) ≥ 3.04·10^-11 forces n ≤ 19. For 5+2i, N0 < q_16 = 56,346,954,291 and |q_15θ - p_15| ≥ 1.77·10^-11, which forces n ≤ 22. With the Laurent bounds the limits are n ≤ 10 and n ≤ 12.

Finite check. An exact search covers n ≤ 60 (7x ± 1) and n ≤ 40 (5+2i, all units ε, all digit pairs (d, e), 226,560 tests). For each n and V it solves (1) or (2) for h, recovers x_0 or z_0, and runs the map to check the shape and the closing. It finds only n = 1 with x_0 = ±1, and n = 1 with z_0 a unit (logs/cyclecircuit.txt). ∎

The proof needs only one of the two Baker statements of Section 6 to be correctly quoted. Matveev's is the weaker, and with it the margin is a factor of about 1.5 below the next convergent.

### 5.5 Two and more circuits

A Simons–de Weger m-cycle analogue would have m runs, each of v = 2 steps (v = 4 steps for 5+2i) with a constant digit d_i, each followed by a descent with digit e_i. Section 5.2 then gives the linear system

  2^(l_i + 2k_{i+1}) h_{i+1} - 7^(k_i) h_i = 2^(l_i+2) d_{i+1} - 7d_i + 3e_i,

whose determinant is ±(2^V - 7^n), and similarly for Z[i]. For m = 2, Cramer's rule gives |2^V - 7^n| ≤ 2^(A_1)|g_2| + 7^(k_2)|g_1|. Relative to 7^n this is of order 4^(-k_i), where k_i is the run after the smaller of the two local minima. So the form is exponentially small only in that run, and a cycle whose other run is short escapes. Simons (Math. Comp. 74 (2005)) handles this for 3x+1 with additional arguments; that was not attempted here.

There is a second obstacle with no 3x+1 counterpart. In 7x ± 1 and 5+2i the digit is a function of the residue class, and it changes freely inside a run of minimal-valuation steps. A typical cycle therefore has about as many circuit breaks as odd steps, and m-cycles with small m are a thin special class. For 3x+1 the digit is always +1, and m counts local minima only. So the circuit route gives Theorem 3, not a general length bound. Simons 2008 (Acta Arith. 131) on generalized Syracuse m-cycles is still unread (prior art, Section 3.1); it should be checked before Theorem 3 is called new.

## 6. Baker-type bounds

### 6.1 What applies

1. Two real logarithms (7x ± 1: ln 2, ln 7; the size form for 5+2i: ln 2, ln 29). M. Laurent, Linear forms in two logarithms and interpolation determinants II, Acta Arith. 133 (2008), 325–348, Corollary 2. The version used is the one restated in arXiv:1510.05579, Lemma 8: for multiplicatively independent real α_1, α_2 > 1 and Λ = b_2 log α_2 - b_1 log α_1, log|Λ| ≥ -17.9 D^4 (max{log b' + 0.38, 30/D, 1/2})^2 h_1 h_2, with h_i ≥ max{h(α_i), |log α_i|/D, 1/D} and b' = |b_1|/(D h_2) + |b_2|/(D h_1). Here D = 1, h(2) is replaced by 1, h(7) = ln 7 and h(29) = ln 29, so b' = (1 + 1/ln 2)n. For n below 10^12 the max is 30, so log|Λ| ≥ -16110 ln 7 = -31,349 and -16110 ln 29 = -54,248. Another version of the same corollary is quoted in arXiv:2208.03068 (constant 20.3, 18/D). Laurent, Mignotte and Nesterenko, J. Number Theory 55 (1995), 285–321 (cited from memory) would serve as well.
2. Any number of logarithms, complex. E. M. Matveev, An explicit lower bound for a homogeneous rational linear form in logarithms of algebraic numbers II, Izv. Math. 64 (2000), 1217–1269, Corollary 2.3, as restated in arXiv:2208.03068, Proposition 14: log|Λ| ≥ -1.4·30^(t+3) t^4.5 D^2 (1 + log D)(1 + log B) A_1 ⋯ A_t, with A_i ≥ max{D h(α_i), |log α_i|, 0.16}.
   - With t = 2 and D = 1, the constant is 1.038·10^9 (ln 2, ln 7) and 1.797·10^9 (ln 2, ln 29); these give the fallback bounds of Theorem 3.
   - For the joint condition of 5+2i, Λ = n log(5+2i) - V log(1+i) - b log i with D = 2, t = 3, A = ln 29, 0.8585 and π/2. This gives log|Λ| ≥ -4.40·10^12 (1 + log B).
3. A. Baker and G. Wüstholz, Logarithmic forms and group varieties, J. reine angew. Math. 442 (1993), 19–62, also covers both cases with worse constants (from memory; not used).

The Laurent and Matveev statements were taken from the restatements named above, not from the original papers.

### 6.2 What they give

For a general cycle, nothing beyond the verification-based bound. The cycle only gives |Λ| ≤ c n/(7X) (resp. c n r). This upper bound does not shrink with n, so a lower bound exp(-C(log n)^2) or n^(-C') yields only n ≥ exp((ln X/C)^(1/2)), which is a trivial bound at X = 10^15. The continued-fraction step of cycle-bounds.md already uses the exact constants, which is the optimal one-dimensional information; for 5+2i the exact scan does the same in two dimensions. Baker bounds become useful only where structure supplies an upper bound on |Λ| that decreases exponentially in n. In the cases worked out here these are the 1-cycles, with |Λ| ≤ C·4^(-n): there Baker gives an absolute upper bound on n, and the convergents reduce it to a finite check (Theorem 3). For m-cycles the upper bound would come from Section 5.5. So for these maps Baker gives a finite-check reduction for special shapes, not a better length bound for general cycles.

## 7. Lattice reduction for the joint condition

1. Separate continued fractions give only the single-condition bounds (169 for size, 161 for angle at B = 3·10^9). The joint condition asks that (nθ_1, ny) be close to Z^2, with y = (β - θ_1)/2 (cycle-bounds.md, Section 5.3), with allowances proportional to n and a parity coupling. That is a simultaneous approximation problem, and continued fractions of θ_1 and β separately cannot decide it.
2. For proving the bound at the current B, the exact scan over n (cyclebound_gauss.py) is exhaustive and already optimal, so neither LLL nor best simultaneous approximation can change 1901 or any value in Section 4. The bound never exceeds the saturation point of order B^(1/2) (about 10^5 at 3·10^9), and in the range of any feasible verification it stays of order B^(1/3) (about 4·10^6 at 10^20), so a scan is always affordable for proofs.
3. For planning, LLL is the better tool. cyclebound_lll.py reduces the lattice with basis (1, [Cθ_1], [Cy]), (0, C, 0), (0, 0, C) for C = 10^3 to 10^27 in steps of 10^(1/8). It collects n from small combinations of the reduced basis and computes r*(n) at 80 digits.
   - Up to 2·10^7 it recovers 41 of the 43 records of the exhaustive scan; it misses 38875 and 12,821,277. (Its extra "record" 54 is an artefact of skipping n < 50.)
   - Beyond the scan it proposes records up to 3.3·10^15. For example, n = 42,445,960 has r* = 5.4·10^-13 (B* ≈ 5.9·10^23), and n = 600,620,502 has B* ≈ 1.6·10^27.
   - These are upper estimates of the thresholds only, since a missed record would lower them.
4. A rigorous lattice version, certifying "no n ≤ N passes" through a lower bound on the lattice minimum in the style of de Weger, would be needed only for B far beyond any verification, and was not done.

## 8. What is proven

Proven, with certified arithmetic, the cited verifications and the quoted Baker bounds as the only inputs:

1. Theorems 1 and 2: the averaged size, angle and disc conditions with c_7 = 0.726219 and c_G = 0.850610 (exact dyadic constants in Section 2.4, certificates reproducible with cyclebound_avg.py). As a side result, the step-size method gives 0.7273 for 3x+1, against Hercher's 3/4.
2. The averaged bounds do not change the current values: 27,855,251 at X = 10^15 and 1901 at B = 3·10^9. They lower the verification needed for the next values:
   - 7x ± 1: 465,645,424 from X = 4·10^15 (Corollary 1; first-order threshold 3.69·10^15, against 5.09·10^15).
   - 5+2i: 2190 from B = 4·10^9 (averaged disc).
   - 5+2i: beyond 8696 from B ≈ 5.0·10^12 (averaged disc), against 6.9·10^12 (basic disc) and 1.32·10^13 (basic box). At B = 1.3·10^11 the bound is 8696 either way.
3. Theorem 3: no non-trivial Steiner-type 1-cycles for 7x ± 1 or for the 5+2i map, unconditionally.

Not proven or not attempted: m-cycles for m ≥ 2 (Section 5.5); a use of residues to beat the step-size constants; a rigorous lattice certificate (Section 7, item 4). The LLL records beyond 2·10^7 and the thresholds of Section 4.1 are planning numbers. The novelty of Theorem 3 depends on Simons 2008, which is unread.
