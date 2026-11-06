# Collatz maps on the Gaussian integers: inventory for the new outline

Compiled 5 November 2026 from the two drafts and the working notes. Nothing here is new mathematics except where an item says "this inventory". Each item gives the content, its status, and its source.

Source key. D1 = `collatz/papers/gaussian-collatz.md` (draft 1). D2 = `collatz/papers/gaussian-collatz-2.md` (draft 2). The rest are in `collatz/research/`: COL = `collatz.md`, LIT = `literature.md`, LAG = `lagarias2016.md`, FAM = `families.md`, CONV = `convergents.md`, SLIT = `symmetric-literature.md`, IC = `integer-census.md`, ST = `symmetry-test.md`, TH = `theorems.md`, LEAN = `lean/README.md` and the files in `lean/GaussianCollatz/`, LOG = `logs/`. A section is written as D2 5.4 or TH Thm 5.4.

Status labels. Lean: proved in Lean 4, axioms at most `propext`, `Classical.choice`, `Quot.sound`. Paper: proved on paper, not formalized. Computer-assisted: a proof that rests on stated program runs. Computed to X: an observation checked for all starts up to the bound X. Heuristic. Literature: a statement taken from a cited source. Open. Superseded: replaced by the item named.

Vocabulary used below, following the brief. A map is the old "expression" or "rule" (multiplier, modulus, digit table). A Collatz map, or rule, is a map meeting triviality: every nonzero orbit reaches a trivial cycle, a cycle containing a unit. A unit cycle is a cycle made only of units. A map commutes with the units if T(uz) = uT(z); the drafts and notes say "symmetric" or "unit-equivariant". Rotated and mirrored maps are those of TH Section 4. Modulus (1+i)^k replaces "level k". Score = 1/M - 1/N. Drift is the mean change of ln N (or log2 N) per odd step.

## 1. Introduction

### 1.1 The lineage from Collatz to this paper

1.1.1. Collatz says he invented the problem before 1952 and circulated it to Hasse in 1952. Literature (Collatz 1986, as reported by Lagarias 2016 p. 772). LAG 1.5, 3 item 5.

1.1.2. Coxeter gave the first published statement in 1971 and offered fifty dollars for a proof and one hundred for a counterexample. Literature (Lagarias 2016 p. 772). LAG 1.2, 1.5, 3 item 4.

1.1.3. Erdős called the problem "hopeless" in conversation and published nothing on it; his only prize in Lagarias 2016 (£10, 1972) was for the density of ⟨2x+1, 3x+1, 6x+1 : 1⟩, solved by Crampin and Hilton; the "mathematics is not ready" remark is not in that paper. Literature, and a caution for the history paragraph. LAG 1.2, 4.

1.1.4. Lagarias's surveys and bibliographies: the 1985 Monthly survey (reference confirmed by the reference list of Lagarias 2016, can lose its [unverified] mark), the 2010 overview (arXiv:2111.02635, no Gaussian content), annotated bibliography I (read to entry 100 in v7, M to Z unread) and II (v2 read in full, current version to entry 89). Literature; reading gaps recorded. LIT 1, 2.11; LAG 3; D2 2.

1.1.5. Lagarias 2016, Erdős, Klarner, and the 3x+1 problem, Amer. Math. Monthly 123(8) (2016) 753-776, doi:10.4169/amer.math.monthly.123.08.753: restates 3x+1 in affine semigroup form; the integral part of the orbit of 4 under ⟨2x, (2x-1)/3⟩ is the backward tree; once noninteger, always noninteger; the orbits are not discrete, so Erdős's density bound does not apply. Literature. LAG 1.5.

1.1.6. The x+1 problem as Lagarias's "solved cousin": the inverse semigroup ⟨2x, 2x-1⟩ is free with orbit of 1 equal to N \ {0}. Literature; used as the comparison for maps below the window. LAG 1.5, 2; D2 3.8.

1.1.7. Klarner and Rado (Pacific J. Math. 53 (1974) 445-463) introduced orbits ⟨R : A⟩ of semigroups of integer affine maps; Klarner-Rado sequence ⟨2x+1, 3x+1 : 1⟩ has density zero (Erdős), contains no infinite progression, and its complement is a union of progressions (Coppersmith 1975). Literature (via LAG, not seen directly). LAG 1.3, 3 items 6, 9; D2 2.

1.1.8. 3x+1 on Z has, besides 0 and 1 → 4 → 2, the cycles -1 → -2 (2 steps), through -5 (5 steps) and through -17 (18 steps); the Böhm and Sontacchi conjecture says these are all; 3x-1 on the positive integers is the same problem as 3x+1 on the negative integers (OEIS A037084, Kamenetsky 2017, Cloitre's three cycles with minima 1, 5, 17); Encyclopaedia of Mathematics records the cycle -5 → -14 → -7 → -20 → -10. Literature. D1 5.1; D2 1; SLIT 3.4.

1.1.9. 3x+1 does not commute with -1: -(3x+1) = 3(-x) - 1, so on the negatives it acts as 3x-1 on the positives; the single conjectured cycle is a property of one half of Z \ {0}. Paper (elementary). D2 1; D1 5.1.

1.1.10. No rule with multiplier 3 and division by 2 can commute with x ↦ -x, because the window 2 < 3 < 4 forces modulus 2 and x ≡ -x (mod 2). Paper (one line). Not found stated in any source read. D1 5.1; D2 3.6; SLIT 3.4.

1.1.11. Generalized maps: Hasse's class (multiply by m, subtract the residue that makes the result divisible by d), Möller (Acta Arith. 34 (1978)), Heppner (Arch. Math. 31 (1978), almost all iterates get smaller when m < d^(d/(d-1))), Matthews and Watts (Acta Arith. 43 (1984); 45 (1985)), Leigh (Acta Arith. 46 (1985)), Matthews and Leigh on F_q[x] (J. Number Theory 25 (1987)). Literature (annotations only; titles and pages partly from memory). LIT 2.7; D1 3; D2 3.4.

1.1.12. The heuristic behind the window: Matthews-Watts Conjecture 3.1 (generalizing Möller) for maps (m_i x - r_i)/d of relatively prime type: almost all trajectories cycle if |m_0 ... m_{d-1}| < d^d, and almost all diverge if the product exceeds d^d; Section 3.2 of Matthews's chapter derives it from a growth-rate argument. The window's upper end is this heuristic carried to the norm, and should be presented so, not as a new criterion. Literature. LIT 2.6, 3.2, 4 item 6; D1 3; D2 3.4.

1.1.13. Matthews's chapter in The Ultimate Challenge (2010) 79-103, Section 10 "Other rings": maps on O_K modulo a non-unit d, worked on Z[√2] (Example 10.1: finitely many cycles found, three one-dimensional invariant sets, divergent orbits), Z[√3] (Example 10.2: 103 cycles, one apparently divergent orbit) and Z[√2] with a unit factor (10.3); no Z[i]; the closing remarks invite systematic study of other rings. Literature. LIT 2.6; D2 2.

1.1.14. Leigh is reported by Siegel as the first to consider Collatz-type maps on rings of algebraic integers (1981 in one source, 1983 in Siegel's withdrawn arXiv:2111.07882), and Matthews knew of no other work on O_K. Literature, unverified (Leigh not seen). COL 1; LIT 2.7, 2.9; D1 11; D2 2.

1.1.15. Kohl's RCWA (GAP package; Dissertation, Stuttgart 2005): rcwa maps of Z^2 with 2x2 integer matrix coefficients and lattice moduli; each step of our maps is such a map (modulus (1+i)^k Z[i]; on even classes the matrix of 1-i over 2). No worked Z^2 example seen. Literature, unverified from source code. LIT 2.8; D2 2.

1.1.16. Siegel's Hydra maps (arXiv:2412.02902; withdrawn arXiv:2111.07882 with examples on Z[√2], Z[√3]; arXiv:2601.17030) cover maps on O_K, with no Z[i] example. Literature. LIT 2.9; D2 2.

1.1.17. Conway (1972) proved a generalization of 3x+1 undecidable; so questions about residue-class-wise affine maps must be settled map by map; Conway 2013 (Amer. Math. Monthly 120, 192-198) on unsettleability. Literature (via Lagarias 2016). LAG 1.5, 3 items 2, 3; D2 2.

1.1.18. Kionke (arXiv:1511.05893; Monatsh. Math. 182 (2017), journal details unverified): maps (m_w x + r_w)/d on cosets of a lattice with positive integer multipliers; tame cones give divergent orbits of positive lower density; on Z[√2] at least half of all points diverge; density-one finite stopping and positive-density divergence can coexist (d ≥ 3); proposes integer matrix multipliers as an open extension, which is where Gaussian multiplication sits. Literature. LIT 2.5; D1 11; D2 2.

1.1.19. Barina, 7x ± 1: Close Relative of the Collatz Problem, arXiv:1807.00908, published CMST 28(4) (2022) 143-147: f(n) = n/2, 7n+1 for n ≡ 1 (mod 4), 7n-1 for n ≡ 3; conjecture that every positive orbit reaches 1; heuristic (1/2)log(7/4) + (1/2)log(1/2) < 0; verified to 10^15; longest orbit below 10^10 has 5717 steps from 6,346,893,259; negative orbits are negated positive ones, only cycles through 1, -1 and 0, contrasted with 3x+1's negative cycles. He does not derive it from a symmetry requirement, frame it by modulus, consider 5x ± 1, or say 3x+1 cannot commute with -1. Literature (read in full). SLIT 2.1.

1.1.20. OEIS entries on Barina's map: A317640 (the 7x ± 1 function, not fetched), A317753 (total stopping time), A318489 (glide), A318972 (shortcut form, with Chai Wah Wu's linear recurrence), the OEIS wiki page "7x±1 problem" (conjectures for positive and negative integers); all link to Matthews's page "David Barina's 7x+1 conjecture", which was not reached. Unrelated 7x+1 entries: A133421, A133422, A267703, A174796. Literature. SLIT 2.2.

1.1.21. Joseph, A chaotic extension of the 3x+1 function to Z_2[i], Fibonacci Quart. 36(4) (1998) 309-316: z/2 on class 0 mod 2, (3z+β)/2 on β ∈ {1, i, 1+i}; restricts to 3x+1 on Z_2; not conjugate to T × T by a Z_2-module isomorphism but topologically conjugate; parity vector map periodic with isometric limit; chaotic in Devaney's sense; no orbits in Z[i] computed. Literature. LIT 2.1; D1 10; D2 2.

1.1.22. Correction: Joseph's page range is 309-316, not 309-317 as in COL 1. Superseded. LIT 2.1.

1.1.23. Kucinski, Cycles for the 3x+1 map on the Gaussian integers, preprint, Scranton, May 2000, 12 pp., under Monks: Joseph's map on Z[i]; Steiner-type period bound in terms of parity vector counts; conjugate subsystems (real axis, imaginary axis, y = x via h(x+yi) = y+xi); no cyclic points in the second and fourth quadrants or on two families of lines; exactly 77 cycles of period at most 400, periods 1, 2, 3, 5, 8, 11, 19, 46, 84, 103 (Maple). Listed by Monks as forthcoming in Pi Mu Epsilon J. (unconfirmed); Monks's slides mention an entire function (Joseph and Monks) extending T on Z[i] to C. Literature, read through text extraction. LIT 2.2; D2 2.

1.1.24. Superseded: COL 1 and D1 earlier text said Kucinski's preprint was "not found online"; it was found. LIT 2.2, 4 item 1.

1.1.25. Alvarado and the 2017 REU at CSU Channel Islands (NSF 1359165; mentors Martinez-Avendano, Martinez, Sittinger; students Knutsen, Ceja Ayala, Torchinsky): talks at Eastern Illinois (1 Dec 2017), Lathisms (2023), Colima (2 April 2025); map C_{2+i,u}: z/(1+i) on even, (2+i)z + u on odd with u a fixed unit, which is our modulus 1+i with α = 2+i; with u = 1, about 5.6% of 23,730 starts with |a|, |b| ≤ 100 reach a unit within 10,000 steps, none between 252 and 10,000 steps; Crandall's heuristic predicts growth; associates proposition (z reaches u_1 under digit u_1 iff u_2 z reaches u_1 u_2 under digit u_1 u_2); closed forms for P_n with C(P_n) = (1+i)^(2n); coalescence mod 5; an Eisenstein analogue dividing by 1-ω without computations; paper said submitted to Minnesota J. Undergrad. Math., not found. Literature, slides only. LIT 2.3; D1 3; D2 2.

1.1.26. del Arte's -5x+1 in OEIS A305057: no source file in the repository mentions it. Gap: needs reading before the introduction can cite it. This inventory. Related material in the notes: integer maps with negative multiplier m = -5 at modulus 4 pass triviality with non-minimal digits (item 2.2.20) and -5 has no unit for 2^v - m (item 2.2.19).

1.1.27. Chamberland's survey "An update on the 3x+1 problem", Section 6.4 (Joseph and Kucinski on Z[i]) and 6.6 (extensions to C). Literature. LIT 2.11; D2 2.

1.1.28. This paper's step: maps on Z[i] dividing by 1+i at moduli (1+i)^k with k ≥ 2, the modulus and digit set as parameters, the window, the criterion of commuting with the units, and the map H; as far as the search reached, none of this is in earlier work. Literature search result, with gaps (bibliography I from M on, Garg's formulas, the REU paper, Matthews's web pages). LIT 3.1 to 3.4; D2 2 last paragraph.

1.1.29. The next ring left open: the Eisenstein integers Z[ω] (window (4^k, 4^(k+1/3)), first map commuting with the units at modulus 4, α = 5+2ω). Computed (see section 4). IC 2.4, 7; ST 10.

### 1.2 What the paper does (results in proof order)

1.2.1. Order suggested by the dependencies in the sources: (a) definitions of map, digit set, accelerated map, triviality (TH Def 1.1 to 1.3, Lemma 1.2, 1.4); (b) the window and the eventually-periodic result below it (D2 Prop 3); (c) no map at modulus 1+i inside the window (D2 Prop 4); (d) modulus (1+i)^2: no map commutes with the units (D2 Prop 2), but five maps of norm 5 are Collatz maps to 10^7 (ST 5.1, TH 6); (e) general facts: at most |U| trivial cycles, rotation and mirroring without the criterion (TH 2.1(a), 4.2, 4.6); (f) existence theorem for maps commuting with the units (IC Thm 1); (g) structure: trivial cycles divide |U|, unit cycles, rotated and mirrored maps, ties (TH 2 to 4, ST 3); (h) fixed-point lemma and Theorem 4, norm 29 least (TH 5); (i) H and its verification to 3·10^9 with the Lean soundness theorem (D2 6, 7, 8); (j) census and drift (D2 5, IC 4); (k) many-cycle maps, Lemma 1 and Theorem 1 (D2 5.6, FAM). This inventory.

1.2.2. Abstract claims of D2: no map commuting with the units at moduli 1 to (1+i)^2; H first single-cycle map in the order by norm, unique at its modulus up to conjugation; moduli (1+i)^3 and (1+i)^5 none; (1+i)^6 and (1+i)^7 have 22 of 112 and 8 of 46; H checked to 3·10^9; multiplier 3 at (1+i)^3 has five cycles; no α = 3 map agrees with 3x+1 on N and contracts; Lean proofs of the soundness, H facts and three propositions. Source text for 1.2. D2 Abstract.

1.2.3. Superseded abstract claims of D1: "the first with a single cycle appears at k = 4 ... among maps with the smallest added terms" (replaced by the order by norm, then by Theorem 4 without minimality); "in each divergent case α/(1+i)^k is real up to a unit" (replaced by the drift along the eight lines, item 4.4.3). D1 Abstract.

### 1.3 Conceptual glossary

1.3.1. Even and odd: z even iff 1+i divides z iff a+b even; (a+bi)/(1+i) = ((a+b) + (b-a)i)/2; dividing by 1+i halves the norm; the exponent of 1+i in an integer n is twice the exponent of 2. Paper. D1 1; D2 3.1; TH 1.

1.3.2. Map (α, k, d): α odd, k ≥ 0, digit table d depending only on z mod (1+i)^k with (1+i)^k | αz + d(z); T and the accelerated map S(z) = w/(1+i)^v(w); S(z) = 0 when w = 0. Definition. TH Def 1.1; D2 3.1; LEAN Expr.lean (`Expr`, `Expr.T`, `Expr.S`).

1.3.3. Digit set D: one odd element in each odd class, 2^(k-1) elements (Lemma 1.2(c): for k ≥ 1 every digit is odd and D has exactly one element per odd class). Lean (`Expr.d_odd`, `Expr.d_inj`). TH Lemma 1.2.

1.3.4. Periodic point, cycle, trivial cycle (contains a unit), unit cycle (only units), trivial map (every odd z has an iterate on a trivial cycle), rule or Collatz map (S trivial). Definition. TH Def 1.3; LEAN Dynamics.lean (`Periodic`, `Reaches`, `OnTrivialCycle`, `Trivial`, `Expr.IsRule`).

1.3.5. Triviality can be read off T or S. Paper. TH Remark 1.5.

1.3.6. Commutes with the units: T(uz) = uT(z) iff d(uz) = u d(z) iff D closed under units (k ≥ 1). Lean (`Expr.Symmetric`, Lemma 1.4(b)). TH Lemma 1.4. Superseded name: "symmetric" (D2 3.2, ST, IC, TH), "unit-equivariant" (COL), "commutes with units" (D1 2).

1.3.7. Rotated map (uα, k, d^u) with d^u(z) = d(uz); mirrored map (ᾱ, k, d̄) with d̄(z) = conj(d(z̄)). Definition. TH Def 4.1, 4.5; LEAN Variants.lean.

1.3.8. Modulus (1+i)^k. Superseded name: "level k" (D1, D2, COL, FAM, IC, ST); TH already says "modulus".

1.3.9. Window 2^k < N(α) < 2^(k+1): expanding in the worst case, contracting on average. Superseded name: "Collatz-like" (D1, D2) for maps in the window; in the new vocabulary "Collatz map" means triviality. The paper needs a separate word for "inside the window". This inventory.

1.3.10. Minimal digit set: each digit of least norm in its class; with ties, every tie-break counts. Superseded as the ordering principle by the score (brief). D2 3.3; ST 2.

1.3.11. Score = 1/M - 1/N. Not defined or computed in any source file; the paper must fix M and N and compute it for the census. Gap. Brief only.

1.3.12. Drift: ln N(α) - (k+1) ln 2 per odd step, or log2 N(α) - (k+1) in bits; for H, log2 29 - 5 = -0.142 bits = ln(29/32) = -0.098. Heuristic quantity. D2 3.4, 6.4; IC 5.

1.3.13. Data of a cycle (n, V, ε): n odd steps, total exponent V, unit ε with S^n(z_0) = εz_0; rel = |1 - α^n/(ε(1+i)^V)|; r = |α|^n/2^(V/2). Definition. FAM 3; D2 5.6.

1.3.14. Drop-below-start check (gstop, gstopall): every start in (N_0, B] reaches a smaller norm, so by induction every z with N(z) ≤ B reaches norm ≤ N_0. Lean (`drop_below_start_sound`, `drop_below_start_sound_all`). D2 7; TH 6.

1.3.15. Superseded name: "expression" (TH, LEAN) for a map with no triviality condition; Lean identifiers `Expr`, `Expr.IsRule` keep it. TH 1.

1.3.16. Associates, units ±1, ±i, norm a²+b², 2 = -i(1+i)², unique factorization. Background. D1 1; D2 3.1.

## 2. Collatz maps without the unit criterion

### 2.1 The map, triviality, the window (definition blocks)

2.1.1. The congruence d(z) ≡ -αz (mod (1+i)^k) depends only on the class of z; α odd is invertible mod (1+i)^k, so z ↦ -αz permutes the 2^(k-1) odd classes. Paper. D1 2; D2 3.1.

2.1.2. Lemma 1.2: (a) 1+i is prime; (b) α odd and (1+i)^k | αx imply (1+i)^k | x; (c) digits odd, one per class. Lean (`pi_dvd_mul`, `pow_dvd_of_odd_mul`, `Expr.d_odd`, `Expr.d_inj`). TH Lemma 1.2.

2.1.3. Lemma 1.4(a): two cycles that meet are equal. Lean (`reaches_back`). TH Lemma 1.4.

2.1.4. Below the window: if N(α) < 2^k every orbit is eventually periodic (ρ = |α|/2^(k/2) < 1, R = δ/(2^(k/2)(1-ρ))). Paper, not in Lean. D1 Prop 2; D2 Prop 3, 4.2.

2.1.5. Worst case: an odd step with exactly k divisions multiplies the norm by N(α)/2^k > 1 above the lower end. Paper. D2 3.4.

2.1.6. In density over odd z, v - k is geometric, P(v - k ≥ j) = 2^(-j), mean 1, variance 2; mean change of ln N per odd step ln N(α) - (k+1) ln 2, negative iff N(α) < 2^(k+1). Paper for the density statement; heuristic for orbits. D1 3; D2 3.4; COL 2.

2.1.7. General residue field of size q: P(v - k ≥ j) = q^(-j), mean 1/(q-1), window q^k < N(α) < q^(k + 1/(q-1)); for q = 2 the paper's window; for Z[ω] (q = 4) (4^k, 4^(k+1/3)), and the naive (4^k, 4^(k+1)) would admit 23 multipliers at modulus 4 instead of 5. Paper (density), heuristic (orbits). IC 2.4.

2.1.8. Odd α has N(α) ≡ 1 (mod 4). Table of windows and multipliers up to units: modulus 1 and 1+i none; (1+i)^2: 5 (2 ± i); (1+i)^3: 9, 13 (3, 3 ± 2i); (1+i)^4: 17, 25, 29 (4 ± i, 5, 4 ± 3i, 5 ± 2i); (1+i)^5: 37, 41, 45, 49, 53, 61 (11 values); (1+i)^6: 65, 73, 81, 85, 89, 97, 101, 109, 113, 117, 121, 125 (28); (1+i)^7: 18 norms 137 to 245 (46). Computed exactly. D1 3; D2 4.4; COL 2, 5 item 1.

2.1.9. The 18 norms at modulus (1+i)^7: 137, 145, 149, 153, 157, 169, 173, 181, 185, 193, 197, 205, 221, 225, 229, 233, 241, 245. Computed. COL 5 item 1.

2.1.10. The windows are disjoint, so a norm determines its modulus, and ordering by modulus then norm equals ordering by norm. Paper. D2 3.5.

2.1.11. Modulus 1 (k = 0): the window (1, 2) holds no integer; the only one-element unit-closed set is {0}, so maps commuting with the units send odd z to αz and never divide. Paper. D1 5.1; D2 Prop 2, 4.1.

2.1.12. For 3x+1 the window reads 2 < 3 < 4 at modulus 2, one digit. Paper. D1 3; D2 3.4.

2.1.13. Below-window example: α = 1+2i (N = 5 < 8) at modulus (1+i)^3 with D = U commutes with the units; |S(z)| ≤ (√5|z| + 1)/2^(3/2) < |z| once |z| > 1.69; every orbit reaches a unit; S(1) = -i, single cycle 1 → -i → -1 → i through all four units; checked for α = 1 ± 2i, 2 ± i for N(z) ≤ 10^5, every orbit reaches a unit within 20 odd steps (`below_window.py`). Paper (contraction) plus computed to 10^5. D2 3.8; TH 5 last paragraph.

2.1.14. Below-window maps are the Gaussian counterpart of the x+1 problem. Remark. D2 3.8.

2.1.15. COL 2 table of contracting multipliers N(α) < 2^(k+1) by modulus (k = 1: units only; k = 2: N = 5; k = 3: 5, 9, 13; k = 4: adds 17, 25, 29); G_{2+i} at (1+i)^3 has provable contraction, single cycle, 59 steps max and excursion 3 bits at 10^4. Computed. COL 2, 3.1.

2.1.16. COL: the contracting maps at (1+i)^3 with unit digits are α ∈ {1, 2+i, 3, 3+2i} up to units and conjugation, drifts ln(5/16) = -1.16, ln(9/16) = -0.58, ln(13/16) = -0.21. Paper. COL 2.

2.1.17. Figure idea: the windows (2^k, 2^(k+1)) on a log scale with the odd norms marked and H's norm 29 highlighted. This inventory.

### 2.2 N and Z: 3x+1, Barina's map

2.2.1. 3x+1 accelerated form, worst case 3/2, average 3/4; measured on x² the factor is 9/16, -0.83 bits per odd step. Paper. D1 1, 4; D2 1, 6.4.

2.2.2. 3x+1 and 3x-1 are both minimal at modulus 2 because ±1 tie in the single odd class (the only tie in Z). Paper. ST Prop 1(3).

2.2.3. In Z the minimal digit set at modulus 2^k, k ≥ 2, is unique and is D_k = {odd d : |d| < 2^(k-1)}; it is also the unique minimal set without the criterion, and S(1) = 1 for every m in the window. Paper. IC Lemma 2.

2.2.4. Integer minimal maps of both signs at moduli 2 to 32, all odd 0 < |x| ≤ 10^6, literal cycles: modulus 2, none of ±3x ± 1 passes (3x+1 has -5, -17; 3x-1 has 5, 17; -3x ± 1 a cycle of 12 odd steps through ±13); modulus 4: m = ±7; modulus 8: none; modulus 16: ±21, ±25; modulus 32: ±41, ±43, ±49, ±53. These are exactly the single-cycle maps of the integer census. Computed to 10^6. ST 6; LOG symtest_Z_k1_B1e6.txt, symtest_Z_k2-5_B1e6.txt.

2.2.5. For m > 0 the trivial cycles are the fixed points 1 and -1; for m < 0 the loop 1 ↔ -1. Paper (Theorem 2.1) and computed. ST 6.

2.2.6. In Z the conjugacy x ↦ -x sends (m, D) to (m, -D); -m is a different map; for D closed under sign, T_{-m} = -T_m on odd steps. Paper. ST 2.

2.2.7. 7x ± 1 (digit +1 for x ≡ 1 mod 4) and 5x ± 1 (digit -1 for x ≡ 1 mod 4) are the two maps at modulus 4 that commute with -1. Paper. D2 3.6; SLIT.

2.2.8. Under 7x ± 1 every odd 3 ≤ x ≤ 10^7 falls below x, so every odd |x| ≤ 10^7 reaches ±1. Computed to 10^7 (`zsym.py`). Superseded as a result by Barina's 10^15; keep only as a check of our code. D2 3.6; SLIT 6.

2.2.9. 5x ± 1 has two cycles up to sign: the fixed point 1 and 7 → 36 → 18 → 9 → 44 → 22 → 11 → 56 → 28 → 14 → 7. Computed. Not found in the literature; OEIS's own search could not be run, so present as "not found", and run an OEIS search for 1, 1, 3, 9, 11, 7, 1, 19, 21 and 4, 1, 16, 2, 24, 3, 36, 4, 44, 5, 56 first. D2 3.6; SLIT 4, 6.

2.2.10. Superseded: D2 3.6 "Integer rules of this kind may already appear in the literature ... which we have not searched"; replace by Barina, Matthews's L_d, Collatz's permutation, Conway's permutation. SLIT 6.

2.2.11. Lemma 3 explains why 7x ± 1 is single-cycle and 5x ± 1 is not only partly: both have δ = 1, so only x = 1 is fixed, but 5x ± 1 has the 3-cycle through 7. Paper plus computation. IC 5.

2.2.12. Pos digit sets (least positive residues 1, 3, ..., 2^k - 1, as in 3x+1): 1 and -1 are fixed for every m (Proposition 4(3)), so no pos map is a Collatz map on Z; x > 0 fixed iff odd x < 2^k/(2^(k+1) - m), so 3 is fixed iff m > (5/3)2^k; x < 0 fixed iff |x| < 2^k/(m - 2^k). Paper. IC Prop 4(3), 6.

2.2.13. Pos maps with m < (5/3)2^k, all odd |x| ≤ 10^5, step cap 10^4: single positive cycle counts by modulus 4 to 1024: 0, 1 (m = 9), 2 (17, 23), 3 (33, 37, 51), 5, 15, 33, 55, 112; single negative cycle 0, 1 (m = 11), 0, 2, 1, 6, 9, 20, 36. Computed to 10^5. IC 6; LOG zcensus_pos_k2-10_B100000_mlt5over3.txt.

2.2.14. For pos maps the smallest multiplier 2^k + 1 has a single positive cycle at every modulus 8 to 1024 (to 10^5), and single-cycle maps crowd to the bottom of the window, the reverse of the maps commuting with -1. Computed to 10^5. IC 6.

2.2.15. Integer non-minimal maps commuting with -1 that pass: modulus 4, digits |d| < 32 (32 maps): only 7x ± 1; modulus 8, 12 candidates (576 maps): 10 pass, none minimal, for example m = 11 with ±1, ±13; m = 11 with ±17, ±3; m = 13 with ±1, ±5; m = 11 with ±47, ±37; modulus 16, 5 candidates (5000 maps): 125, 2 minimal. Computed to 10^6. ST 7; LOG symtest_Z_symnonmin_*.

2.2.16. Integer non-minimal maps not commuting with -1, modulus 4, 80 random digit sets |d| ≤ 15, m = ±5, ±7: 6 pass, all with m < 0, e.g. m = -5 with digits 13, 3 and m = -7 with -3, 11. Computed to 10^6. ST 8 item 2; LOG symtest_Z_nonmin_k2.txt.

2.2.17. Unit loops 1 ↔ -1 in Z: needs m + d_a = -2^v and -m + d_b = 2^(v'); at modulus 2 only -3x ± 1, which fail; modulus 4: 82 maps with the loop, 5 pass (-7x ± 1 and four with m = -5: digits (1, 11), (-11, -1), (-3, 59), (-59, 3)); modulus 8: 2500 run, 106 pass (m = -9: 102, -11: 3, -13: 1), two commute with -1; modulus 16: sample of 3000 of 75816, 201 pass, including m = 21 with digits 17, 3, 37, 23, -7, -37, 13, -1, and m = 25; 7x ± 11 has the loop but fails. Computed to 10^6. ST 9; LOG symtest_Z_loop_*.

2.2.18. Minimal digits with m > 0 give S(1) > 0, so a unit loop needs m < 0 or a non-minimal digit. Paper. ST 9.

2.2.19. For m < 0 the numbers 2^v - m are never units, so Corollary 3a gives no constraint; for m > 0 at modulus 4, Corollary 3a forces digits ±1. Paper. ST 8 item 2.

2.2.20. The cycle of 3x+1 through -17 has n = 7, V = 11; 11/7 is an intermediate fraction of log2 3, a record of the one-sided problem 2^V < 3^n forced for x < 0; the cycles at -1, 1, -5 sit at records; 5x+1: 13 and 17 at n = 3 (7/3, side 2^V > 5^n); 181x+1: 27 and 35 at n = 2 (15/2; 181² = 2^15 - 7, attributed to Crandall); no other cycles for m ≤ 31. Computed to |min| ≤ 2·10^5. CONV 5; LOG convergents_int_plus.txt.

2.2.21. Collatz's original permutation g (2n/3, (4n-1)/3, (4n+1)/3 by n mod 3) is odd (Tremblay, arXiv:1907.13302 eq. (1)); Conway's amusical permutation A006368 has a(n) = -a(-n), odd branch (3x ± 1)/4 with m = 3 below the window and an even branch 3x/2; Matthews's L_d (via Carnielli, Appl. Math. E-Notes 15 (2015); Tremblay eq. (43)) with least absolute residues is odd for odd d, orbits reported to reach 1 or -1 by sign. Literature. SLIT 3.1 to 3.3.

2.2.22. Lu Pei's L_3: every 1 ≤ x < 10^5 reaches 1; L_5, L_7 have further cycles. Computed to 10^5 (scratchpad `lupei.py`, not in the repository). SLIT 3.3.

2.2.23. Correction: COL Sources attributed arXiv:1907.13302 and 1910.11798 to Matthews; both are by Tremblay and concern Z. Superseded. LIT 2.13.

2.2.24. Works checked and found to be on Z only, with no Gaussian, lattice or number-field map (Tremblay x3, Carnielli, Goodwin, Hohertz-Kalantari, Khanzadeh Holasou, Wiltrout-Landquist, Rozier, Gilbert, Carelli, Zhu-Chen-Sun, OEIS wiki 3x+1, Clark and Lewis; Bruschi; Rorabaugh) and with no symmetric map (MathWorld Matthews map, Edgington, Briscese-Calogero, Goyal, Shrimali-Antu-Jones, arXiv:0707.1746, Barina-Maat, Bouhamidi, Ye, Siegel 1909.09733, Behajaina-Paran). Literature. LIT 2.13; SLIT 3.5; COL 1.

2.2.25. Villatoro's popular survey (2020) lists Barina's conjecture; no research paper found citing Barina. Literature. SLIT 2.3.

### 2.3 Z[i] at modulus 1+i: impossible

2.3.1. No map at modulus 1+i lies in the window: N(α) = 3 is not a sum of two squares. Lean (`prop3_int`, `prop3`; the step from the window to N(α) = 3 is not formalized). D1 Prop 3; D2 Prop 4.

2.3.2. The direct analogue of 3x+1 (one odd class, one digit) does not exist in Z[i], for the same reason that 3 stays prime. Paper. D1 3; D2 4.3.

2.3.3. The REU map C_{2+i,u} is at modulus 1+i with N(α) = 5 above (2, 4), drift ln 5 - 2 ln 2 > 0 per odd step, consistent with the 5.6%. Paper (drift) and literature (the 5.6%). D2 4.3; LIT 2.3.

2.3.4. The REU associates proposition is the modulus 1+i content of the fact that no map there commutes with the units: units permute the maps instead of fixing one. Remark. LIT 2.3, 3.2.

2.3.5. At modulus 1+i the four units are tied in the single odd class. Paper. ST Prop 1(1).

2.3.6. COL k = 1 remark: "N < 4: units only, so every non-trivial single-rule map diverges on average". Superseded wording, same content as 2.3.1. COL 2.

### 2.4 Z[i] at modulus (1+i)^2: the 2-i family and its unit cycle

2.4.1. No map at modulus (1+i)^2 commutes with the units: -z ≡ z mod (1+i)^2, so d(-z) = d(z) = -d(z) forces d = 0. Lean (`prop1`, `pi_sq_dvd_two_mul`). D2 Prop 2; D1 Prop 1; TH Thm 5.4 case k = 2.

2.4.2. Weaker form also fails: S(-z) cannot always be an associate of S(z), since (αz + d)/(αz - d) is within 1/2 of 1 for large |z| and no unit times a power of 1+i other than 1 is. Paper. D1 5.1; D2 4.1.

2.4.3. Multipliers 2 ± i and associates (8); 2+i has the smallest odd prime norm, the closest analogue of 3. Remark. D1 3; D2 4.4.

2.4.4. Minimal digit sets at modulus (1+i)^2: the classes of 1 and i are tied with {±1}, {±i}; 4 minimal sets, none commuting with the units, forming one unit orbit; so 8 maps up to conjugacy, one per associate, D = {1, i}. Paper (ties) and computed. ST Prop 1(2), 3 table, 5.1.

2.4.5. Of the 8, five are Collatz maps: 2-i (unit cycle 1 → -i → -1 → 1), -1-2i (1 → -1 → -i → 1), -2-i (1 → i → -1 → -i → 1), 1+2i (fixed -1, -i), -2+i (fixed 1, -i); three fail: 2+i (cycle through 5-2i, 1-6i, -5-4i, -6+3i, 7i), 1-2i (two 2-cycles), -1+2i (five cycles). Computed to 10^5; the four digit sets of each multiplier give identical counts. ST 5.1; LOG symtest_k2_B1e5.txt.

2.4.6. The five passing maps checked by gstopall over all four quadrants for 1 < N(z) ≤ 10^7, 15,708,048 starts each, no failure, no cycle; max stopping time 203, 210, 216, 260, 236 steps of T; mean 7.76 to 7.77; max excursion 10.06 to 11.38 bits. Computed to 10^7, independent Python check to 10^4. ST 5.1; LOG symtest_k2_gstopall_1e7.txt (per-map figures only in the log).

2.4.7. The conjugate pair 2-i and 2+i get opposite verdicts: conjugation is not a symmetry of maps that do not commute with the units. Computed. ST 5.1.

2.4.8. The map E_2: α = 2-i, digit i for a odd b even, 1 for a even b odd; drift ln(5/8) = -0.47 per odd step, the strongest contraction inside the window; largest excursion 11.4 bits. Computed. ST 5.1.

2.4.9. E_2 is a map (digit table correct), divides on even inputs, S(1) = -i, S(-i) = -1, S(-1) = 1, S(i) = -i, T(z) ≠ 0 for z ≠ 0. Lean (`digit2_class`, `digit2_correct`, `E2_T_div`, `E2_cycle`, `E2_T_ne_zero`). TH Prop 6.2.

2.4.10. E_2 has a unit cycle of length 3, not the order of any unit, and the unit i lies off it and is not periodic; both conclusions of the unit-cycle theorem fail without the criterion. Lean (`E2_cycle`, `E2_I_not_periodic`, `E2_not_symmetric`). TH Remark 3.2.

2.4.11. Corollary: given the gstopall check with N_0 = 1 and bound B, every nonzero z with N(z) ≤ B reaches {1, -i, -1}; with B = 10^7 from the run. Lean (`E2_sound`) plus computer-assisted. TH Cor 6.3.

2.4.12. Where E_2 orbits enter its trivial cycle: of 141,324 odd starts with N(z) ≤ 90000 (excluding the cycle and i), 76,511 enter at 1, 24,683 at -1, 40,130 at -i, and 17,979 pass through i. Computed (`entry2mi.py`, run for this inventory; not written up anywhere).

2.4.13. These five maps are the Gaussian counterpart of 3x+1 that the criterion rules out: one or two trivial cycles and no other, at the first modulus with multipliers in the window, with norm 5 against 29 for H. Remark. ST 1 item 1, 5.1.

2.4.14. Non-commuting sweeps at modulus (1+i)^2 with digits of norm ≤ 5 chosen freely: all 288 maps converge, 68 have a single nonzero cycle. Computed to 10^5 (`sweep_gen.py`). D1 5.1; D2 3.2; COL 3.2.

2.4.15. All single-cycle maps at (1+i)^2 found in the sweeps do not commute with the units. Remark. D1 5.1.

### 2.5 Z[i] at modulus (1+i)^4

2.5.1. Minimal digit sets at (1+i)^4: 4 tied classes (pairs {1 ± 2i}, {±2 + i} and unit images, norm 5), 16 minimal sets, 2 commuting with the units, 6 orbits under D ↦ uD (Burnside (16+4+2+2)/4). Paper and computed. ST 3.

2.5.2. All 28 associates × 6 digit-set orbits = 168 minimal maps at N(z) ≤ 10^5: 35 pass; 8 commute with the units (H and its mirror in four associates each); 27 do not: 16 of norm 17 (associates ±1 ± 4i, 4 ± i), 11 of norm 29; none of norm 25. Computed to 10^5. ST 5.2; LOG symtest_k4_B1e5.txt.

2.5.3. Under the criterion the norm-17 multipliers have two cycles; a non-commuting tie-break removes the second. Computed. ST 5.2.

2.5.4. α = 1+4i and 4+i with D = U ∪ {1-2i, -2+i, -2-i, -1-2i} pass gstopall to N(z) ≤ 10^6. Computed to 10^6. ST 5.2.

2.5.5. The associates -4 ± i, -3-4i, -4-3i can never pass (Corollary 3a with ε = 1). Paper. ST 5.2, 7.

2.5.6. A non-commuting modulus-(1+i)^4 map with 3 trivial cycles (fixed units 1, -1, i): α = -2-5i, D = U ∪ {1-2i, -2+i, 2-i, -1-2i}; 3 does not divide 4, so the divisor statement needs the criterion. Computed. ST 4; TH 2.

2.5.7. All 128 minimal tie-breaks for the eight associates and conjugates of 5+2i: 48 are Collatz maps at N(z) ≤ 10^5 (same at 2·10^4): 5+2i 13, i(5+2i) = -2+5i 7, -5-2i 1, 2-5i 11, 5-2i 1, 2+5i 9, -5+2i 1, -2-5i 5; 8 commute with the units, 40 do not. Trivial cycles among the non-commuting ones include unit cycles of length 3 (2+5i: 1 → -i → i → 1 and -1 → i → -i → -1), a lone unit 2-cycle {i, -i} or {-i, i}, and three fixed units (-2-5i). Computed to 10^5 (`variants29.py`, LOG variants29_1e5.txt, variants29_2e4.txt). Only TH 4 mentions the script; the counts are not written up.

2.5.8. In 2.5.7 the associates of 5+2i (32 passing) and of 5-2i (16 passing) differ, another case of mirroring failing without the criterion; -5-2i, 5-2i and -5+2i pass only with the tie-break that commutes with the units. Computed. This inventory from LOG variants29_1e5.txt.

2.5.9. Non-minimal non-commuting sample at (1+i)^4: 80 random digit sets with digits of norm ≤ 25 in their classes and random associates: 12 pass, at norms 17 (7), 25 (3) and 29 (2); norm 25 now has passing maps (-3+4i, 3+4i, 4-3i). Computed to 10^5. ST 8 item 1; LOG symtest_k4_nonmin_sample.txt.

2.5.10. Non-commuting sweeps at (1+i)^4 with digits of norm ≤ 5: 448 maps, 384 converge (all failures α = ±5, ±5i), 76 with a single nonzero cycle. Computed to 10^5. D1 5.1; D2 3.2; COL 3.2.

### 2.6 General facts: the trivial-cycle bound, rotation without the criterion

2.6.1. Any map on Z[i] has at most |U| = 4 trivial cycles. Lean (`trivial_cycles_le_four`, `trivial_cycle_has_unit`, `Expr.trivial_cycles_le_four`). TH Thm 2.1(a); ST Thm 2(1).

2.6.2. Counts of trivial cycles found among passing maps: 1 or 2 at (1+i)^2; 1, 2, 3 (non-commuting) and 4 (commuting) at (1+i)^4; 1, 2, 3, 4 at (1+i)^6 (one sampled map with 3). Computed. ST 4.

2.6.3. Rotation without the criterion: S_{uα}(z) = S_α(uz). Lean (`Expr.rotate_S`). TH Prop 4.2.

2.6.4. Mirroring without the criterion: S_ᾱ(z) = (-i)^v conj(S_α(z̄)) with v the exponent of w = αz̄ + d(z̄), and N(S_ᾱ(z)) = N(S_α(z̄)). Lean (`Expr.mirror_S`, `Expr.mirror_norm`). TH Prop 4.6.

2.6.5. Only one step relates: without the criterion the iterates of rotated or mirrored maps are not related, because the unit (-i)^v from conj(z/(1+i)) = i z̄/(1+i) is absorbed only by a map that commutes with the units; complex conjugation is not a conjugacy, and a non-commuting map depends on the choice of generator 1+i of the prime. Paper. ST 2; TH 4.

2.6.6. z ↦ uz is a conjugacy carrying (α, D) to (α, u^(-1)D); the four unit images of a digit set always give identical results. Paper and computed. ST 2.

2.6.7. Lemma 4.3 (transfer): if g commutes with units and g(z) = ε f(z) with a unit ε for every z, then f trivial implies g trivial. Lean (`trivial_transfer`). TH Lemma 4.3.

2.6.8. An f-invariant set of non-units containing an odd element rules out triviality. Lean (`not_trivial_of_invariant`). LEAN Dynamics.lean.

2.6.9. Fixed-point lemma with ε = 1 needs no criterion: if (π^v - α)z ∈ D, v ≥ k, z odd non-unit, then z is a nontrivial fixed point. Lean (`Expr.fixed_point`, `Expr.fixed_point_not_rule`). TH Lemma 5.1; ST Prop 3.

2.6.10. Soundness of the all-quadrant drop-below-start check for any T dividing by π on even inputs, no condition on units, at four times the starts. Lean (`drop_below_start_sound_all`). TH Thm 6.1; ST 12 (`gstopall.c`).

## 3. Collatz maps that commute with the units

### 3.1 The criterion and the case for it

3.1.1. A conjecture of Collatz type fails by a second cycle or a divergent orbit; Z shows that a map meant to carry such a conjecture on a ring larger than N should commute with the units and count cycles up to units. Argument. D2 1.

3.1.2. A map that commutes with the units has no half: every odd Gaussian integer is a unit times a primary one; Conjecture 1 then speaks about all of Z[i] \ {0}, and the check covers every nonzero start. Paper. D1 5.1.

3.1.3. Maps that do not commute with the units often have a single cycle (68 of 288, 108 of 972, 76 of 448 at (1+i)^2, (1+i)^3, (1+i)^4), so a single cycle alone does not pick out a map. Computed to 10^5. D1 5.1; D2 3.2; COL 3.2.

3.1.4. Commuting with the units does not follow from triviality, not even with minimality: five minimal maps at (1+i)^2 and 27 at (1+i)^4 pass without it. Computed (see 2.4.5, 2.5.2). ST 1 item 1, 11.

3.1.5. Under the criterion, triviality alone (any digits) puts the first map inside the window at norm 29 (Theorem 4, item 3.4.6). Paper. ST 11; TH 5.

3.1.6. Literature: no source proposes commuting with the units as a requirement or classifies such maps by modulus; Barina states the mirror property as a consequence; Tremblay states Collatz's permutation is odd; Matthews's L_d are symmetric by construction ("balancing"). Literature negative, with the gap that Hasse, Möller, Heppner, Leigh, Wirsching and most chapters of The Ultimate Challenge were not seen. SLIT 5.

3.1.7. The order on maps: compare by N(α); minimal digit sets first among maps with the same multiplier. Superseded in part: minimality as the order on digit sets is replaced by the score. D2 3.5; brief.

3.1.8. Superseded framing (COL 4 (c), D1 5.1): "the claim that H is special rests on minimality (smallest k) and uniqueness at that k, not on single-cycleness". Replaced by "H comes first in the order by norm" (D2 5.1) and then by Theorem 4 (no minimality needed). COL 4; D1 5.1; D2 5.1; TH 5.

3.1.9. Superseded D1 framing: "What singles out H is the smallest level at which a single-cycle rule exists, together with uniqueness at that level". D1 5.1.

### 3.2 Existence (Z from modulus 4, Z[i] from (1+i)^3)

3.2.1. Theorem 1 (Dedekind domain R, prime P with residue field of size q, finite unit group U): equivalent are (1) a map commuting with U at modulus P^k for some odd α; (2) for every odd α; (3) no u ≠ 1 has u ≡ 1 mod P^k, i.e. k > κ = max v_P(u-1); (4) U acts freely on the odd classes. Paper. IC Thm 1.

3.2.2. Up to the choice inside a class, a map commuting with the units is a choice of one element in one class of each orbit; the number of minimal such sets is the product over orbits of the number of least-norm elements in a class of that orbit; this gives 1, 2, 1, 4, 1 at (1+i)^3 to (1+i)^7. Paper. IC 2.2.

3.2.3. Thresholds: Z (κ = 1, from modulus 4); Z[i] (κ = 2, from (1+i)^3); Z[ω] with P = (2) inert (κ = 1, from modulus 4); other imaginary quadratic fields κ = e (modulus P^2 if 2 unramified, P^3 if ramified); infinite unit groups: no modulus works, though {±1} alone is possible from k > e. Paper. IC 2.3.

3.2.4. Z[i]: for k ≥ 3 the units lie in distinct classes (differences have v ≤ 2), act freely, 2^(k-3) orbits of four; a digit set commuting with the units is the unit multiples of one digit per orbit. Paper. D1 2; D2 3.3.

3.2.5. No map at moduli 1 to (1+i)^2 commutes with the units (Lean `prop1`); at modulus 1 the only one sends odd z to αz. Lean and paper. D2 Prop 2.

3.2.6. Lean scope note: Proposition 2 is formalized as "for k ≤ 2 a digit function depending only on the class mod π^k with odd values has d(-z) ≠ -d(z) for some odd z". Lean. D2 8; LEAN Props.lean.

3.2.7. Z from modulus 4: 5x ± 1 and 7x ± 1 (items 2.2.7 to 2.2.9). Paper and computed. D2 3.6.

3.2.8. Existence holds for every odd α as soon as for one. Paper. IC Thm 1.

### 3.3 Structure: trivial cycles divide |U|, unit cycles, rotated and mirrored maps, smallest digits and the criterion (ties)

3.3.1. Trivial cycles under the criterion: if 1 is not periodic no unit is and there are none; otherwise the trivial cycles are uC_1, the stabilizer is U ∩ C_1 (a subgroup), and their number is |U|/|U ∩ C_1|: four (stabilizer {1}), two ({±1}) or one (U). Lean (`unit_periodic_iff`, `unit_cycle_eq`, `stab_iff`, `stab_mul`, `trivial_cycle_count`, `Expr.trivial_cycle_count`). TH Thm 2.1(b); ST Thm 2(2).

3.3.2. Examples: H has two ({1, -1}, {i, -i}; stabilizer {±1}); -5-2i with H's digits has four fixed units; 2-5i with H's digits has one cycle through all four units; the below-window map 1+2i at (1+i)^3 has one; 7x ± 1 two fixed points, -7x ± 1 the loop 1 ↔ -1; Eisenstein 5+2ω and associates 1, 2, 3 or 6. Computed and paper. ST 4 table.

3.3.3. The count depends on the associate of α: replacing α by uα multiplies each odd step by u. Paper. ST 4.

3.3.4. Unit cycles: if w = S(1) is a unit then S(u) = uw, S^n(u) = uw^n, every unit lies on a unit cycle of length ord(w), and there are 4/ord(w) of them; if w is not a unit there is no unit cycle. Lean (`unit_cycles`, `no_unit_cycle`, `Expr.unit_cycles`). TH Thm 3.1.

3.3.5. Rotation under the criterion: the rotated map commutes with the units, S_{uα} = uS_α, S^n_{uα} = u^n S^n_α, same norm sequences, rule iff the original is, S_{uα}(1) = uw. Lean (`Expr.rotate_symmetric`, `rotate_S_mul`, `rotate_iter`, `rotate_norm`, `rotate_rule`, `rotate_S_one`). TH Thm 4.4.

3.3.6. Mirroring under the criterion: S^n_ᾱ(z) = (-i)^(V_n(z̄)) conj(S^n_α(z̄)), norm sequence of z under the mirror equals that of z̄ under the original, rule iff the original is. Lean (`Expr.mirror_symmetric`, `mirror_iter`, `mirror_norm_iter`, `mirror_rule`). TH Thm 4.7.

3.3.7. If D is closed under conjugation, (α, D) and (ᾱ, D) are mirror images; in general (α, D) and (ᾱ, D̄). Paper. D1 2; D2 3.2.

3.3.8. The eight rotated and mirrored maps of H (four rotations of H, four of (5-2i, U ∪ (2-i)U)) all are Collatz maps iff H is; numerical check (`mirror29.py`): 58,560 comparisons of 60-step norm sequences over odd |a|, |b| ≤ 60, no mismatch. Lean (theorems) and computed (check run for this inventory; the count is not written up). TH 4.

3.3.9. Lemma 1 (ST): if every odd class has a unique least-norm element, the minimal digit set is unique and commutes with the units. Paper. ST Lemma 1.

3.3.10. Ties (Voronoi): least-norm elements lie in the closed Voronoi cell of (1+i)^k Z[i]; ties only on its boundary; odd k: square |x| + |y| ≤ 2^j, boundary even for j ≥ 1, so no ties (except modulus 1+i); even k = 2j: 2^j tied classes with two least elements each, 2^(2^j) minimal sets, 2^(2^(j-2)) commuting with the units (j ≥ 2); Z: only ±1 at modulus 2. Paper. ST Prop 1, 3.

3.3.11. Ties table: modulus (1+i)^2: 2 odd classes, 2 tied, 4 minimal sets, 0 commuting, 1 orbit; (1+i)^3: 4, 0, 1, 1, 1; (1+i)^4: 8, 4 (norm 5), 16, 2, 6; (1+i)^5: 16, 0, 1, 1, 1; (1+i)^6: 32, 8 (four of norm 17, four of 25), 256, 4, 70; (1+i)^7: 64, 0, 1, 1, 1; Z moduli 4 to 64: unique. Computed (`symtest.py ties`) and paper. ST 3.

3.3.12. So at odd moduli minimality forces the criterion and triviality adds nothing to the census; only even moduli carry minimal maps that do not commute with the units. Paper. ST 3, 11.

3.3.13. Minimal digit sets commuting with the units by modulus: (1+i)^3: the units; (1+i)^4: U with the unit multiples of 2+i, or of 1+2i (conjugate); (1+i)^5: unit multiples of 1, 3, 1+2i, 1-2i; (1+i)^6: unit multiples of 1, 3, 1 ± 2i, 2 ± 3i with those of 1+4i or 1-4i and of 3+4i or 3-4i (four sets, none closed under conjugation); (1+i)^7: unit multiples of 1, 3, 5, 7, 1 ± 2i, 1 ± 4i, 1 ± 6i, 2 ± 3i, 2 ± 5i, 3 ± 4i (unique, closed under conjugation). Computed exactly; enumeration reproduced by an independent division test. D2 4.4; COL 5 item 1.

3.3.14. Level-6 digit-set names: A = 1, 3, 1 ± 2i, 2 ± 3i, 1-4i, 3-4i; B = A with 3+4i; C = A with 1+4i; D = A with 1+4i and 3+4i; A, D and B, C are mirror pairs. Definition. FAM 2.

3.3.15. Superseded: D1 2 definition "a digit set is minimal if each chosen digit has the least norm in its class" applied to digit sets commuting with the units only; ST extends it to all tie-breaks; the brief replaces minimality by the score. D1 2; ST 2.

3.3.16. Symmetry and triviality do not force minimality: passing digit sets share no visible feature beyond avoiding F_α (digits up to 47 at integer modulus 8, outside the Voronoi cell). Computed. ST 7.

### 3.4 Theorem 4: norm 29 is the least possible inside the window

3.4.1. Fixed-class lemma: S(z) = εz with exponent v iff v ≥ k and (επ^v - α)z ∈ D; under the criterion or with ε = 1 and z an odd non-unit, the cycle of z avoids the units. Lean (`Expr.fixed_point`, `Expr.fixed_point_not_rule`). TH Lemma 5.1; IC Lemma 3; ST Prop 3.

3.4.2. Corollary: if επ^v - α = η is a unit for some v ≥ k (ε = 1 without the criterion), triviality forces every digit to be a unit; so it fails for every digit set once there are more odd classes than units (from (1+i)^4 in Z[i], modulus 8 in Z). Lean (`Expr.digits_units`). TH Cor 5.2; ST Cor 3a.

3.4.3. Table of multipliers covered by the corollary (v ≤ k+2): commuting maps at (1+i)^3: all; (1+i)^4: 4 ± i, 5, 4 ± 3i (all but 5 ± 2i); (1+i)^5: 5 ± 4i, 7; (1+i)^6: 8 ± i, 9, 8 ± 7i; (1+i)^7: 9 ± 8i, 15; Z: 2^k + 1, 2^(k+1) - 1; non-commuting literal associates: (1+i)^2: -2+i, -1+2i, 1+2i; (1+i)^3: -3, -3+2i, -2+3i; (1+i)^4: -4 ± i, -5, -4-3i, -3-4i; (1+i)^5: -5-4i, -4-5i, -7i; (1+i)^6: ±1-8i, -9i, 7-8i, 8-7i; (1+i)^7: 8-9i, 9-8i, 15. Computed. ST 7.

3.4.4. Lemma 5.3: N(α) < 29 inside the window forces k ∈ {2, 3, 4} and α an associate of 2 ± i; 3, 3 ± 2i; 4 ± i, 5, 4 ± 3i. Lean (`alpha_k3`, `alpha_k4`). TH Lemma 5.3.

3.4.5. The η table: k = 3: α = 3 (ε = -1, v = 4, η = 1), 3+2i (-i, 3, -1), 3-2i (-1, 3, -1); k = 4: 4+i (-1, 4, -i), 4-i (-1, 4, i), 5 (-1, 4, -1), 4+3i (-1, 5, i), 4-3i (i, 5, -i). Lean (by `decide`). TH 5.

3.4.6. Theorem 4: a map commuting with the units with 2^k < N(α) < 2^(k+1) and N(α) < 29 is not a Collatz map, for any digit table. At (1+i)^4 all digits units is impossible (take z_0 = 1+2i); at (1+i)^3 D = U and explicit cycles: α = 3: -2-i → 1+2i → 1-2i = i(-2-i); α = 3+2i: -3 → 1+4i → 5-2i → -3+2i → 3; α = 3-2i: -3 → 4+i → -5-2i → 2-3i → 3i; literal cycles of 8, 8, 16 elements. Lean (`Expr.no_symmetric_rule_below_29`, `units_mod_pi3`, `Expr.no_rule_of_cycle3`, `Expr.no_rule_k4`). TH Thm 5.4; ST Thm 4.

3.4.7. The theorem uses no minimality; with minimal digits H and its mirror are the only norm-29 maps commuting with the units that pass up to 10^9 (3·10^9 for H). Paper plus computed. ST Thm 4; TH 5.

3.4.8. In Z at modulus 4, a map commuting with -1 that is a Collatz map has digits ±1 (both m = 5, 7 covered by the corollary). Paper. ST Thm 4.

3.4.9. The window is a hypothesis that cannot be dropped: α = 1+2i at (1+i)^3 also gets D = U by the corollary (ε = -i, v = 3, η = 1), and it is a Collatz map. Paper. TH 5.

3.4.10. ST's proof of Theorem 4 relied on the census for D = U at (1+i)^3; TH replaces that by the displayed multiplications, checked in Lean. Superseded proof route. ST Thm 4; TH 5, 7.

3.4.11. Non-minimal maps commuting with the units at (1+i)^3: digits among the 40 smallest of a class per orbit, 120 maps, none passes; at (1+i)^4 with 5 candidates per orbit, 175 maps, 3 pass, 2 minimal (H and mirror) and α = -5-2i with D = U ∪ (5-2i)U (four fixed units). Computed to 10^5. ST 7; LOG symtest_G_symnonmin_k3-4.txt, symtest_G_symnonmin_k3_40.txt.

3.4.12. α = -5-2i with D = U ∪ (5-2i)U: gstop over 1 < N(z) ≤ 10^7, 3,927,012 starts, no failure; max stopping time 9754 at 1782+243i, mean 53.91 (the same mean as H), max excursion 80.35 bits at 2728+1543i. Computed to 10^7. ST 7; LOG symtest_k4_a-5-2i_D5-2i_gstop_1e7.txt (the mean and locations only in the log).

3.4.13. D1 and D2 earlier: at (1+i)^3 the non-minimal digit sets of the unit multiples of 2+i, 2-i or 3 give 5 to 14 cycles each. Computed to 10^5. D1 5, 5.1; D2 5.2; COL 3.1.

3.4.14. No passing map has a digit in F_α = {(επ^v - α)z : v ≥ k, z odd, N(z) > 1}; F_α accounts for 3728 of 4981 failures among integer maps at moduli 4 to 16, 464 of 566 at modulus 8 with 12 candidates, and 147 of 187 among Gaussian maps at (1+i)^3, (1+i)^4; the rest come from longer cycles. Computed. ST 7.

3.4.15. Lemma 3 instances: 8 ± i at (1+i)^6 lies at distance 1 from 8 = i(1+i)^6, so every digit gives a fixed class (at least 8 per digit set, matching four maps with 8 cycles); 7 ± 4i at distance √17 has a single cycle; 11+4i at (1+i)^7: (8+8i) - (11+4i) = -3+4i, only z = 1 passes; structural fixed points α = 9 at (1+i)^6 (8 - 9 = -1) and 15 at (1+i)^7 (16 - 15 = 1); H: (4+4i) - (5+2i) = i(2+i) ∈ D gives the fixed class of 1 with exponent 5 and nothing else. Paper plus computed. IC 5.

### 3.5 The 5+2i rule

3.5.1. Definition: α = 5+2i, N = 29, modulus (1+i)^4 = -4, so classes mod 4; 8 odd classes in two unit orbits (units and unit multiples of 1+2i); D = {±1, ±i, ±(2+i), ±(1-2i)}. Definition. D1 4; D2 6.1.

3.5.2. Digit table: z ≡ 1, i, -1, -i, 1+2i, 2+i, -1-2i, 2-i (mod 4) get -1+2i, -2-i, 1-2i, 2+i, -1, -i, 1, i. Lean (`pi_pow_four`, `four_dvd_class`, `tableH_reps`, `tableH_distinct`, `tableH_covers`, `digitH_correct`). D2 6.1; LEAN RuleH.lean.

3.5.3. Derivation from one congruence: α ≡ 1+2i mod 4, (1+2i)² ≡ 1; digits alternate between units and associates of 2+i; multiplication by α swaps the two orbits. Paper. D2 6.1.

3.5.4. (5+2i)·1 + (-1+2i) = 4+4i = -(1+i)^5, so S(1) = -1 and S(-1) = 1. Lean (`H_cycle`). D2 6.1.

3.5.5. H divides on even inputs and commutes with the units. Lean (`H_div`, `digitH_unit_table`, `digitH_unit`, `divPi_unit_mul`, `H_unit`). D2 8; LEAN HSound.lean.

3.5.6. The other minimal digit set gives the mirror image, α = 5-2i. Paper. D2 6.1.

3.5.7. Where 3x+1 has one digit, H has eight, because the window forces (1+i)^4. Remark. D1 4; D2 6.1.

3.5.8. H is the largest norm at its modulus, as 7x ± 1 is the largest multiplier at modulus 4 in Z; the next modulus has none in both rings ((1+i)^5, modulus 8). Computed. D2 3.6; IC 1 item 6, 5.

3.5.9. Orbit of 3 under S: 3 → -4-i → 4+3i → -3+2i → 2-3i → -4+3i → -2-i → -i, norms 9, 17, 25, 13, 13, 25, 5, 1, exponents 4, 4, 6, 5, 4, 7, 7. Computed. D1 4; D2 6.2.

3.5.10. 6827+4464i takes 2042 odd steps to reach a unit (also the chunk-1 record stopping time 11271 steps of T and excursion 97.67 bits). Computed. D1 4; D2 6.2; COL 3.6; LOG alpha5p2i_k4.stop.

3.5.11. Conjecture 1: every orbit of H on Z[i] \ {0} reaches ±1 or ±i; up to units a single cycle; on Z[i] the cycles through ±1 and through ±i. Open. D1 4; D2 1.

3.5.12. Backward form: H with all four divisions at once has inverse branches w ↦ (1+i)w and w ↦ (-4w - d)/(5+2i), d ∈ D; the integers in the semigroup orbit of {1, i} are the backward tree (a non-integer never returns, since 4 and d are prime to 5+2i); Conjecture 1 says this is Z[i] \ {0}; -1, -i are images of 1, i; the orbit is not discrete (norm factor 16/29 < 1), so Erdős's density bound does not apply. Paper (from Lagarias 2016's form). D2 6.3; LAG 2.

3.5.13. The inverse branches of these maps are affine maps of Z^2, i.e. 3x3 integer matrices (where Klarner-Birget-Satterfield prove freeness undecidable) and 2x2 upper triangular over Z[i] (the open case of Cassaigne-Harju-Karhumäki); neither result settles freeness here. Remark (LAG author's own observation). LAG 2, 3 items 7, 8.

3.5.14. Coppersmith's feedback elements and covering criterion could ask whether complements of inverse orbits are unions of classes mod powers of 1+i. Remark, flagged as a stretch. LAG 3 item 9.

3.5.15. Verification: gcz classified every N(z) ≤ 10^7 (only the cycles through ±1, ±i); gstop over 10^7 < N(z) ≤ 3·10^9 in 30 chunks, 1.18·10^9 starts, no failure, no other cycle, about 15 minutes on one core; coordinates below 2^80; norm comparisons exact while below 10^36. Computed to 3·10^9. D2 7; D1 12; COL 3.6; LOG alpha5p2i_k4.stop.

3.5.16. H_sound: if every odd z with a > 0, b ≥ 0, N_0 < N(z) ≤ B has an iterate of smaller norm, every z with N(z) ≤ B has an iterate of norm ≤ N_0. Lean. Trusted: the gstop run with N_0 = 10^7, B = 3·10^9, the gcz run below 10^7, the C programs, 128-bit arithmetic, the compiler, and agreement of the rule file `alpha5p2i_k4.rule` with `digitH` (checked entry by entry). D2 7, 8.

3.5.17. Statistics to 3·10^9: longest stopping time 13913 steps of T at 6742+34011i (chunk (1.2·10^9, 1.3·10^9]); mean 53.9 (53.82 to 53.93 per chunk); record excursion 122.9 bits at 17467+5226i; to 10^9 the longest stopping time is 12226 (at 8.1·10^8 to 9.1·10^8). Computed. D1 4; D2 7, 6.4; LOG alpha5p2i_k4.stop.

3.5.18. Census-run figures for H at N(z) ≤ 10^6: max total steps 6365, max excursion 74.6 bits; the 5-2i map of the census table (H's multiplier class with the other digit set, 3 cycles) has max total 6659 and max excursion 90.5. Computed. LOG k345_1e6_rerun.txt (not in prose).

3.5.19. Drift: at least four divisions (factor 16) against 29, on average five (32); factor 29/32 per odd step, -0.142 bits, against 9/16 (-0.83 bits) for 3x+1 on x²; H contracts about six times more slowly. Heuristic. D1 4; D2 6.4.

3.5.20. Random-walk model: log2 N walks with drift -0.142 and variance 2; simulating 4·10^6 walks gives mean stopping time 53.8 steps of T against 53.82 to 53.93 measured. Heuristic, matched. D1 9; D2 6.4.

3.5.21. Excursion tail ≈ C e^(-θh) with θ = 0.165 per bit solving E[e^(θX)] = 1 for X = log2 29 - 4 - G; diffusion value 2μ/σ² = 0.142 is a little low; C ≈ 0.36 (peak after multiplication, before divisions); median record over n starts (ln n + ln C - ln ln 2)/θ = 122.4 bits for n = 1.18·10^9 against 122.9 observed. Heuristic. D1 9; D2 6.4.

3.5.22. The agreement is closer than the model justifies: the record in 14 of 30 chunks comes from orbits through one point of norm about 2^151.2 (checked for five); the mean stopping time is the better test. Computed. D1 9; D2 6.4.

3.5.23. Superseded: COL 2 and 3.6 estimate θ = 0.142 with n = 4·10^7 (one chunk), "about 123 bits", and "122.9 including the 4.86-bit pre-division peak"; "the model is accurate". Replaced by 3.5.21 and 3.5.22. D1 9 note.

3.5.24. Contrast with the modulus-(1+i)^7 single-cycle maps: drift -0.90 bits (norm 137) to -0.41 (193); to 10^9 their excursions are 2^9.7, 2^14.8, 2^29.2, 2^37.4 and longest stopping times 284, 512, 1596, 2243; below 10^6 below 2^25; H over the same range 2^122.9 and 12226. Computed to 10^9. D2 6.4; COL 5 item 6.

3.5.25. Among the maps commuting with the units with a single cycle known, H has the weakest drift apart from the four (1+i)^6 maps with α = 11 ± 2i. Computed. D2 6.4.

3.5.26. A cycle of H with n odd steps and exponent V satisfies ∏(α + d_j/z_j) = ε(1+i)^V; for large z_j this forces 2^V ≈ 29^n and arg (5+2i)^n ≈ Vπ/4 mod π/2; the angle condition has no analogue for 3x+1; Steiner and Simons-de Weger bounds might be adapted; the quantity is |ε(1+i)^V - α^n|. Open. D1 13 item 1; D2 9 item 1.

3.5.27. 2-adic extension: the digit depends only on z mod 4, so H extends to Z_2[i]; for 3x+1 the extension preserves Haar measure and is conjugate to the shift (Lagarias 1985, Bernstein 1994); open for H. Open. D1 13 item 4; D2 9 item 7.

3.5.28. Push H to 10^11: about 1.2·10^6 starts per second per core, about nine core-hours. Open (resource estimate). D1 13 item 5; D2 9 item 8; COL 4 (b).

3.5.29. Literature: no source considers α = 5+2i, modulus (1+i)^4 or this digit set, or conjectures a single cycle for a Gaussian map. Literature negative with gaps. LIT 3.3.

3.5.30. Superseded D1 4 wording: "H is unique there up to conjugation" stated with "level"; and D1's "H" defined among minimal digit sets only. Replaced by D2 6 and Theorem 4. D1 4.

### 3.6 The census, score and drift

3.6.1. Census method: maps commuting with the units, in the window, every minimal digit set; counted up to units and up to conjugation; gcz follows every start to its cycle; every cycle re-iterated in exact arithmetic (`exact.py`, digit from the congruence) before grouping; counts are lower bounds (cycles reached from N(z) ≤ B), except the complete lists of Theorem 1. Computed. D2 5.1; COL 5 item 10.

3.6.2. Summary table (D2): moduli 1 to (1+i)^2: 0 maps; (1+i)^3: 3 (2), single 0, cycles 5, 4, to 10^9; (1+i)^4: 14 (7), single 2 (1) = H, others 2, 2, 3, 3, 6, α = 5: 35 or more and divergent, to 10^6 (H to 3·10^9); (1+i)^5: 11 (6), single 0, cycles 2, 3, 3, 4, 4, 18, to 10^6; (1+i)^6: 112 (56), single 22 (11), others 2 to 10, α = 9: 23, 25, α = 11: 560, 568 (at least 639 for one digit set), both divergent, to 10^6; (1+i)^7: 46 (24), single 8 (4), others 2 to 33, α = 13, 15, 11 ± 10i: 166, 74, 766, to 10^6 (single-cycle and Theorem 1 maps to 10^9). Computed. D2 5.1.

3.6.3. Superseded D1 table entries: (1+i)^7 "4 (8)" single-cycle (should be 8 (4)); "2 to 545; none divergent"; "checked to 10^6"; and the columns counted up to units only for moduli 6 and 7. D1 5.1.

3.6.4. Modulus (1+i)^3: G_3 (α = 3, D = U) has 5 cycles up to units: 2+i → 1+2i (2 odd steps, 92.3% of starts), 5+2i → 2+5i (2, 5.6%), 19+6i → 10+19i → 22+7i → 11+22i → 25+8i (5, 1.0%), its mirror through 6+19i (5, 1.0%), the fixed class of 1 (1, 0.2%); valuations (3, 3), (3, 3), (3, 3, 3, 3, 4), 1 with v = 4. Computed to 10^9. D1 6; D2 3.7; COL 3.1, 3.5.

3.6.5. G_3 at 10^6: max steps 269, max excursion 7.2 bits; to 10^9 (gstop from 10^6): max stopping time 196, mean 9.17, max excursion 7.93; share ending at 1 falls from 0.2% (10^6) to 0.07% (10^7). Computed. COL 3.1, 3.6; LOG G3_k3.stop, k345_1e6_rerun.txt.

3.6.6. G_{3+2i}: 4 cycles: fixed class of 1 (63.4%, v = 3), 3 → 1+4i → 2+5i → 2+3i (4 odd, 22.9%, v 3, 3, 5, 4), 7 → 2+9i → 8+3i → 11+2i (4, 12.8%, v 3, 4, 3, 5), 8+7i → 12+7i → 17+6i → 10+13i (4, 0.8%, v 3, 3, 4, 5); at 10^6 max steps 1474, excursion 28.6; to 10^9 max stopping 2426, mean 22.5, excursion 43.4. G_{3-2i} is its mirror. Computed to 10^9. COL 3.1, 3.5, 3.6; LOG G3p2i_k3.stop.

3.6.7. Correction: the gstop runs for G_3 and G_{3+2i} cover (10^6, 10^9] with 392,306,389 starts each (3.92·10^8, as D1 says); D2 7 and COL 3.6 write 3.93·10^8. Superseded value. LOG G3_k3.stop, G3p2i_k3.stop.

3.6.8. G_3's mean factor 9/16 is the square of 3x+1's 3/4; G_3 resembles 3x+1 on all of Z (more than one cycle). Remark. D1 6; D2 3.7; COL 3.1.

3.6.9. Modulus (1+i)^4 table (digit set U ∪ (2+i)U; the other set gives mirrors): 4+i: 2 cycles (1, 2+i, both 1 odd step); 4-i: 2; 5: 35 or more and divergent orbits; 4+3i: 3 (1, 6+3i of 11 odd steps); 4-3i: 6 (1, 4+i of 12); 5+2i: 1; 5-2i: 3 (1, 3+4i of 6, 15+16i of 70). Computed to 10^6. D1 5; D2 5.2.

3.6.10. Basin shares at (1+i)^4 from the log: 4 ± i: 96.7% and 3.3% or 53.6% and 46.4% by digit set; α = 5: 329,124 starts escaped at 10^6 (10.5%), max excursion 25.9; 4 ± 3i: 58.2%, 41.0%, 0.8% or 34.0%, 31.7%, 16.1%, 9.2%, 6.9%; 5-2i: 77.9%, 18.2%, 3.9% for the 70-cycle. Computed to 10^6. LOG k345_1e6_rerun.txt (not in prose).

3.6.11. Superseded counts for α = 5 at (1+i)^4: "36+" (COL 3.3, not reproduced with cap 50000), then 34 (4 Nov rerun), then 35 after the 40-cycle print limit was fixed. COL 3.3, 5 item 3; D2 5.2.

3.6.12. Superseded: COL 3.3 gives the 70-cycle of 5-2i through 16+15i; D1 and D2 give 15+16i (first-quadrant representative). Check which is the canonical element. COL 3.3; D2 5.2.

3.6.13. Modulus (1+i)^5 table: 6+i: 2 (1, 5 of 4 odd steps); 5+4i: 4 (1, 2+i of 1); 6+3i: 3 (1, 2+i of 2); 7: 18 (1, 2+i of 1); 7+2i: 4 (1, 2+i of 3); 6+5i: 3 (1, 2+i of 7, 164+199i of 130); mirrors 6-i, 5-4i, 6-3i, 7-2i, 6-5i; α = 7 its own mirror. Computed to 10^6; same at 10^5. D1 5; D2 5.2.

3.6.14. Superseded: COL 3.3 said the 130-cycle of 6+5i goes through 199+164i; the rerun shows 164+199i, and 199+164i is on the 6-5i cycle. COL 3.3.

3.6.15. 6+5i (drift ln(61/64)) has excursions of 2^112 (COL) or 117.8 bits (census log) and max total 27,206 steps at 10^6. Computed. COL 3.3; LOG k345_1e6_rerun.txt. Not in either draft.

3.6.16. Proposition 5 (D2): among the maps commuting with the units, minimal, in the window with k ≤ 5, every map other than H and its mirror has at least two cycles up to units, by the exhibited cycles verified in exact arithmetic. Computer-assisted (exhibited cycles). D2 Prop 5; D1 Prop 4. Not in Lean.

3.6.17. Modulus (1+i)^6: 28 multipliers, 4 digit sets, 112 maps, 56 up to conjugation, none its own mirror; 22 single-cycle up to units at 10^6: α = 7 ± 4i, 8 ± 3i, 9 ± 2i, 10 ± i, 10 ± 3i, 11 ± 2i. Computed to 10^6. D1 5; D2 5.3; COL 3.3.

3.6.18. The 45 multi-cycle maps up to conjugation: 19 with 2 cycles, 4 with 3, 3 with 4, 5 with 5, one each 6 and 7, 4 with 8, 2 with 9, 2 with 10; then α = 9 (23, 25) and α = 11 (560, 568 at 10^6; 181, 211 at 10^5). For α = 10 ± 5i and the two 11 ± 2i maps with two cycles, starts hit the cap 50000 (lower bounds). Computed to 10^6. D2 5.3; COL 3.3.

3.6.19. α = 11 ± 2i (norm 125, drift ln(125/128)): excursions pass gcz's 2^200 norm escape; gstop from norm 1 to 10^6 with escape 2^110; 26 escaped starts iterated in unbounded arithmetic until below their norm, up to 138,239 steps of T, excursions up to about 2^300; single cycle to 10^6. Computed to 10^6. D1 5; D2 5.3, 7; COL 3.3.

3.6.20. Superseded: α = 11 at (1+i)^6 "23 or 24 cycles" (print limit of 40 cycles in quiet mode); and "the same counts hold at 10^5 and 10^6" for α = 11. COL 5 item 3; D1 5, 5.1.

3.6.21. Modulus (1+i)^7: 64 odd classes in 16 orbits, unique minimal set closed under conjugation, 46 maps, 24 up to conjugation (13, 15 self-mirror); every start ≤ 10^6 reached a cycle, no cap, no escape; largest excursion 2^153.7 (14 ± 7i). Computed to 10^6. D2 5.4; COL 5 items 1, 5.

3.6.22. Single-cycle maps at (1+i)^7: 11 ± 4i (137), 12 ± 3i (153), 11 ± 8i (185), 12 ± 7i (193); fixed class of 1, one odd step, 8 steps of T (9 for 12 ± 3i); gstop for all eight from norm 1 to 10^6 (392,686 starts each), then for -11+4i, -12+3i, -11+8i, -12+7i over (10^6, 10^9], 392,306,389 starts each, no failure, 9 min 39 s; mean stopping times 17.0, 18.4, 28.4, 30.6. Computed to 10^9. D2 5.4, 7; COL 5 items 4, 6; LOG k7_single_gstop_1e6.txt, k7_single_gstop_1e9.txt.

3.6.23. Same norm, different behaviour: 9 ± 8i and 12 ± i (both norm 145) have 16 and 2 cycles; 12 ± 3i (153) has one. Computed. D2 5.4; COL 5 item 9.

3.6.24. Cycle counts of the other 20 maps at (1+i)^7: 2 (12 ± i); 3 for nine (10 ± 9i, 13 ± 4i, 14 ± 3i, 13 ± 6i, 14 ± 5i, 12 ± 9i, 13 ± 8i, 15 ± 4i, 14 ± 7i); 4 (13 ± 2i); 5 (11 ± 6i, 14 ± i); 8 (12 ± 5i); 11 (10 ± 7i); 16 (9 ± 8i); 33 (15 ± 2i); 74 (15); 166 (13); 766 (11 ± 10i); conjugates agree. Computed. D2 5.4; COL 5 item 7.

3.6.25. Superseded (1+i)^7 counts at 10^6: 72 for 15 (now 74), 545 for 11 ± 10i (now 766); growth 10^5 to 10^6: 141 to 166, 64 to 72, 149 to 545. COL 5 items 7, 8; FAM 7 item 1.

3.6.26. Long cycles at (1+i)^7: mirror pair of 537 odd steps for α = 15 through 4081+4664i and 4664+4081i; 21 odd steps (181 T-steps) through 11+4i for 14+i; 75 odd steps (661 T-steps) through 7 ± 4i for 15. Computed. D2 5.4; COL 5 item 7.

3.6.27. Fractions of single-cycle maps: Z[i] 0/3, 2/14, 0/11, 22/112, 8/46 at (1+i)^3 to (1+i)^7 (14%, 0%, 20%, 17% from (1+i)^4). Computed. IC 5; COL 5 item 9.

3.6.28. Single-cycle drifts: H ln(29/32) = -0.098; (1+i)^6: -0.68 to -0.024 (11 ± 2i); (1+i)^7: ln(N/256) between -0.63 and -0.28 (= -0.90 to -0.41 bits). Computed. IC 5; COL 5 item 9; D2 6.4.

3.6.29. Integer census of maps commuting with -1 (D_k digits), moduli 4 to 1024, 1022 maps: single-cycle counts 1, 0, 2, 4, 5, 15, 30, 67, 127 of 2, 4, 8, ..., 512; 251 in all; each single-cycle map checked by drop-below-start for odd |x| ≤ 10^7; every other map has two cycles verified (765 by `zverify.py`) or given by Proposition 4. Computer-assisted (Proposition 6). IC 1, 4, 8; LOG zsummary_sym.txt, zcensus_sym_drop_1e7.txt, zverify.txt.

3.6.30. Integer census table columns: first single-cycle m by modulus (7, none, 21, 41, 77, 153, 307, 611, 1223); smallest m cycles (5: 2, 9: 2, 17: 4, 33: 8, 65: 16, 129: 32, 257: 64, 513: 128, 1025: 256); largest m cycles (7: 1, 15: 3, 31: 7, 63: 12, 127: 16, 255: 32, 511: 64, 1023: 128, 2047: 256); excluded by Proposition 4 (0, 2, 2, 6, 10, 22, 42, 86, 170); most cycles (5: 2, 15: 3, 31: 7, 63: 12, 97: 50, 181: 294, then 257/511, 513/1023, 1025/2047). Computed to 10^6. IC 4.2.

3.6.31. Integer single-cycle lists: modulus 4: 7; 16: 21, 25; 32: 41, 43, 49, 53; 64: 77, 85, 99, 107, 111; 128: 153, 155, 157, 165, 167, 173, 193, 195, 197, 201, 207, 211, 213, 227, 229. Computed. IC 4.2; LOG zsummary_sym.txt.

3.6.32. Integer modulus 8 cycles: m = 9: fixed 1, 3; 11: 1 and a 2-cycle through 5; 13: 1 and 10 odd steps through 151; 15: 1, 3 and 43 odd steps through 89. Computed. IC 4.2.

3.6.33. Proposition 4 (integers): under D_k, odd x > 0 is fixed iff x < 2^(k-1)/δ with δ = min(m - 2^k, 2^(k+1) - m); no S(x) = -x; fixed points up to sign ⌊(2^(k-1)/δ + 1)/2⌋, at least 2 iff m/2^k < 7/6 or > 11/6; m = 2^k + 1 and 2^(k+1) - 1 have 2^(k-2) fixed points (every positive digit). Paper; matches the census for all 1016 maps run. IC Prop 4, 3.2; LOG zsummary_sym.txt.

3.6.34. Integer single-cycle maps lie in m/2^k ∈ [1.193, 1.832] inside (7/6, 11/6); drift in [-0.5164, -0.0877] against the bound (ln(7/12), ln(11/12)) = (-0.539, -0.087). Computed and paper. IC 4.3.

3.6.35. By tenths of the window: single-cycle/maps 0/102, 7/102, 45/103, 46/102, 0/102, 51/102, 40/102, 47/103, 15/102, 0/96; tenth 4 (around √2) is empty because of 2-cycles with V = 2k+1 (at modulus 1024 every m from 1391 to 1469 has one, 53 at m = 1449 where 1449²/2^21 = 1.0012); thinner gaps near 2^(1/3), 2^(2/3), 2^(1/4), 2^(3/4) (at 1024 skips 1255 to 1309 and 1611 to 1653). Computed. IC 4.3. Figure idea: histogram by tenths.

3.6.36. Integer proportion of single-cycle maps settles near a quarter from modulus 128 on (23%, 23%, 26%, 25%). Computed. IC 5.

3.6.37. Z versus Z[i]: first commuting modulus 4 versus (1+i)^3; first single-cycle at the first commuting modulus in Z, at the second in Z[i]; both first single-cycle maps are the largest multiplier at their modulus; the smallest multiplier 2^k + 1 is never single-cycle in Z for k ≥ 3 (Prop 4) but is in Z[i] at (1+i)^6 (7 ± 4i, norm 65) and (1+i)^7 (11 ± 4i, 137), explained by Lemma 3 (distance to ε(1+i)^k, ε(1+i)^(k+1) rather than to 2^k). Paper and computed. IC 1 item 6, 5.

3.6.38. Z: no divergence at any modulus to 1024 (escape threshold 2^2000; peaks up to 2^1322 for m = 255; with a 2^100 threshold many maps near the top seemed to escape, e.g. 10^4 starts for m = 61, 63, all returned exactly); eleven maps hit the step cap (127; 255; 509, 511; 1017 to 1023; 2031, 2033, 2035), all in the range of Proposition 4, drift -0.0083 to -0.0010. Computed. IC 4.2.

3.6.39. Superseded integer hint: an escape threshold of 2^100 suggested divergence near the window tops; exact iteration removed it. IC 4.2.

3.6.40. Z modulus 1024, m ≥ 2033 (drift -0.0073 to -0.0005): too slow at 10^6; 2033 and 2035 at 10^5 have exactly the 17 and 20 fixed points of Proposition 4; 1 and 26 starts hit the cap. Computed to 10^5. IC 4.1; LOG zcensus_sym_k10top_B100000.txt.

3.6.41. Integer census method: `zcz census sym`, odd 1 ≤ x ≤ 10^6, memo on [-10^6, 10^6], Brent, 128-bit with multi-word fallback above 2^100, step cap 10^5 odd steps plus 10^6 under Brent; cross-checked with pure Python for m = 31 at modulus 16. Computed. IC 4.1.

3.6.42. Drift comparison of first single-cycle maps: 7x ± 1 -0.134, H -0.098, Eisenstein 5+2ω -0.290 (natural log). Heuristic quantities. IC 7.

3.6.43. Score = 1/M - 1/N: no values computed anywhere; the census tables would need a score column once M and N are fixed. Gap. Brief.

3.6.44. Figure idea: single-cycle maps at (1+i)^6 and (1+i)^7 plotted by norm and drift, with H and the 11 ± 2i maps marked. This inventory.

### 3.7 Maps with many cycles

3.7.1. Maps studied: (1+i)^7: α = 13, 15, 11 ± 10i, 15 ± 2i; (1+i)^6: α = 9, 11 with all four digit sets. No infinite family; no affine map relates two cycles except mirror symmetry; cycles spread over all angles; no shared parity vector beyond most valuations equal k. Computed. FAM 1; D2 5.6.

3.7.2. Cycle equation: z_0(ε(1+i)^V - α^n) = Σ d_j α^(n-1-j) (1+i)^(V - s_j); |z_0| of order 1/rel; large cycles need α^n close to ε(1+i)^V; for real α one condition (V/(2n) ≈ log2 α), for non-real two (modulus and angle). Paper. FAM 3; D2 5.6.

3.7.3. Near-coincidences: 13^10 = 1.00305·2^37 (rel 3.05·10^-3, class (10, 74, -i), 160 cycles, E = 117); 15^43 = 0.9974·2^168 (2.56·10^-3, (43, 336, -1), 32, 29.3); (11+10i)^33 near a unit times (1+i)^257 (6.17·10^-4, (33, 257, i), 728, 764); (15+2i)^6 near i(1+i)^47 (1.58·10^-2, (6, 47, i), 28, 20.5); 11^37 = 0.99929·2^128 (7.1·10^-4, (37, 256, -1), 498 or 481 at 10^6, 613); 9^6 = 1.0136·2^19 (1.36·10^-2, (6, 38, i), 15 or 17, 18.4). Computed; E heuristic. FAM 1, 3; D2 5.6.

3.7.4. Full class distributions at the final bounds: 13: (10, 74, -i) 160, (3, 22, -i) 2, (20, 148, -1) 2, fixed 1 (1, 8), (2, 14, i) through 1+2i; 11+10i: (33, 257, i) 728, (66, 514, -1) 29, (132, 1028, 1) 3, (99, 771, -i) 2, (198, 1542, -1) 1, (264, 2056, 1) 1, (1, 7) and (2, 16) one each; 15: (43, 336, -1) 32, (1, 8, -1) 16, (32, 250, -i) 7, (75, 586, i) 6, (247, 1930, i) 4, (107, 836, 1) 2, (11, 86, -i) 2, (8, 62, i) 2, (537, 4196, 1) 2, (10, 78, i) 1; 15+2i: (6, 47, i) 28, (1, 8, -1) 3, (5, 39, -i) 1, (12, 94, -1) 1; 9 at (1+i)^6: (6, 38, i) 15 or 17, (1, 6, -i) 8; 11 at (1+i)^6: (37, 256, -1) 498 or 481, (74, 512, 1) 46 or 51, (2, 14, i) 7, a few each of (13, 90), (111, 768), (148, 1024), (11, 76), (185, 1280), (222, 1536), (209, 1446). Computed. FAM 2.

3.7.5. Lengths as sums of resonant lengths: for 15, 75 = 43 + 32, 107 = 75 + 32, 247 = 5·43 + 32, 537 = 12·43 + 21; for 11+10i, multiples 66, 132. Observation. FAM 2; D2 5.6.

3.7.6. Structural fixed points: for α = 15 at (1+i)^7 every digit d gives a fixed point (with α = -15 and z = -d, αz + d = 16d = (1+i)^8 d up to a unit), 16 in all; α = 9 at (1+i)^6, S(d) = -id, 8 fixed points; the heuristic gives 16 and 8 exactly. Paper. FAM 2, 3; D2 5.6.

3.7.7. Heuristic count E(n, V, ε) = 2^((k-1)n) C(V - kn + n - 1, n - 1)/(4n N(ε(1+i)^V - α^n)) from the branch count (2^((k-1)n) classes of odd (1+i)-adics with given valuations, each with one point satisfying S^n(z) = εz); matches main classes within a factor of about 1.4; further rows: 13 (20, 148, -1) 2.8 vs 2; 11+10i (66, 514) 44.6 vs 29, (99, 771) 7.1 vs 2, (132, 1028) 1.7 vs 3; 15 (32, 250) 1.6 vs 7. Heuristic. FAM 3; D2 5.6; LOG families_theory_classes.txt.

3.7.8. Totals of E over n ≤ 20000, |V - n log2 N| ≤ 3, all ε: 129 (13), 70 (15), 30.5 (15+2i), 825 (11+10i), 31 (9), 721 (11 at (1+i)^6); constant from n = 1000 (3000 for 11); convergence by large deviations (V/n near log2 N(α) below the mean k+1); predicts finitely many cycles. Heuristic. FAM 3; LOG families_heur.txt.

3.7.9. Partial sums of E by n cutoff (100, 300, 1000, 3000, 20000): 13 constant 129.3; 15: 65.7, 68.8, 70.2, 70.2, 70.2; 15+2i: 30.4, 30.5, ...; 11+10i: 822.2, 824.8, ...; 9: 31.1; 11: 687.9, 707.9, 718.2, 721.2, 721.2. Computed (heuristic sums). LOG families_heur.txt.

3.7.10. For 11+10i both conditions hold at n = 33 to about 6·10^-4, far better than the generic two-dimensional Dirichlet bound n^(-1/2); classes at multiples m n_0 have rel about m rel_0 and E falls (764, 44.6, 7.1, 1.7, 0.5, ...). Heuristic. FAM 3.

3.7.11. Lemma 1 (bound on one element): with g_j = ln|α| - (v_j/2) ln 2, c = max(0, ln r), G_m = min(m(ln|α| - (k/2) ln 2), c) for m < n, G_n = ln r, some element has |z| ≤ (d_max/(|α| rel)) Σ e^(G_m); proof by starting at a prefix-minimum index (cycle lemma). Paper. FAM 5; D2 Lemma 1.

3.7.12. Lemma 2: if |V - n log2 N(α)| ≥ 3 the bound is at most 1.55 d_max n/|α|, so per n only six V and four units need evaluating; d_max = 7 at (1+i)^7, 5 at (1+i)^6. Paper. FAM 5; D2 5.6.

3.7.13. Coverage of Lemma 1: 13: n ≤ 156 at 1.3·10^7 (157: 1.6·10^7), n ≤ 216 at 10^9 (217: 1.4·10^9); 15: n ≤ 246 at 10^9 (247: 7.1·10^9); 15+2i: n ≤ 982 at 2.85·10^7, n ≤ 1498 at 10^9; 11+10i: all n ≤ 2000 (largest 6.4·10^8, nearly constant about 6.3·10^8 along n = 33m); 11 at (1+i)^6: n ≤ 578 at 10^9 (579: 1.2·10^9), bound for n = 37 about 5.6·10^8. Computed (double precision, margin ≥ 1.4). FAM 5; LOG families_cover.txt.

3.7.14. Ratio of least norm to the bound: at most 0.29 (13), 0.27 (11+10i), 0.51 (15+2i); for 15 it reaches 1.0 at the fixed point 7, where the bound is attained. Computed. FAM 5.

3.7.15. Theorem 1 (computer-assisted): at (1+i)^7 with the minimal digit set, 13 has exactly 166 cycles with least norm ≤ 10^9, including every cycle with ≤ 216 odd steps, 160 of 10 steps; 15 has 74, every cycle ≤ 246, 32 of 43; 11 ± 10i has 766, every cycle ≤ 2000, 728 of 33, 29 of 66, 2 of 99, 3 of 132; 15 ± 2i has 33, every cycle ≤ 1498, 28 of 6. Computer-assisted. D2 Thm 1; FAM 5.

3.7.16. Proof runs: gcz to 1.3·10^7, 1.65·10^7, 10^7, 2.85·10^7 (MAXC raised to 65536 in a scratchpad copy `gczbig`); gstop to 10^9: 13 over (1.3·10^7, 10^9], 387,593,990 starts, no cycle, 128 s; 11+10i over (10^7, 10^9], 388,772,062 starts, one cycle with least element 2318+2599i (norm 1.21·10^7, already known), 329 s; 15 over (1.65·10^7, 10^9], 386,219,564 starts, 8 capped (311+4664i, 326+4891i, 359+5382i, 2679+3062i and mirrors), each reaching the 537-step pair within 1 to 21 odd steps without dropping, 370 s; 15+2i over (2.85·10^7, 10^9], 381,507,118 starts, no cycle, 419 s. Computer-assisted. FAM 5; D2 5.6; LOG families_gstop_*.

3.7.17. Stopping statistics of those gstop runs (not in prose): 13 max stopping 841, mean 21.48, max excursion 21.26 bits; 11+10i max 9518, mean 55.54, excursion 74.86; 15 max 21409, mean 62.90, excursion 84.06; 15+2i max 13931, mean 72.12, excursion 92.10. Computed to 10^9. LOG families_gstop_k7_summary_1e9.txt.

3.7.18. Exact verification (`famverify.py`): every listed cycle's digits recovered as (1+i)^(v_j) z_(j+1) - αz_j lie in D with (1+i)^k dividing αz_j + d_j, elements odd, cycle equation holds, elements pairwise non-associate; 166, 74, 33, 766, 4·(23 to 25) and 4·(560 to 568) cycles, none failed; rel² = N(den)/2^V exactly: 9.318·10^-6, 3.808·10^-7, 6.576·10^-6, 2.503·10^-4. Computed exactly. FAM 5; D2 5.6.

3.7.19. Number of cycles with least norm ≤ B (10^2 to 10^8): 13: 2, 6, 30, 127, 166, 166, 166; 15: 20, 29, 40, 62, 72, 72, 74; 15+2i: 5, 21, 33, ...; 11+10i: 2, 3, 6, 66, 395, 765, 766; 9 (set A): 9, 13, 23, 23, 23; 11 (set A): 8, 11, 26, 98, 445, 568; largest least norm 2.7·10^5 (13), 1.2·10^7 (11+10i), 3.84·10^7 (15, the 537 pair). Computed. FAM 2. Figure idea: saturation curves of these counts.

3.7.20. α = 11 at (1+i)^6 has no complete list: gstop over least norms (10^6, 3·10^6] for set A found 172 cycles, 101 already known, so at least 639 (same for the mirror); out of reach because many starts fall into cycles above their own norm and run to the cap (about 13,000 starts per second). Computed. FAM 2; D2 5.6; LOG families_gstop_k6_a11_A_1e6_3e6.txt.

3.7.21. Heuristic expectation of missing cycles beyond n = 300: about 1.4 for 15 (the 537 pair shows such cycles occur), below 0.1 for the others, about 13 for 11 at (1+i)^6. Heuristic. FAM 5.

3.7.22. Structure tests (`famstruct.py`): only conjugation composed with a unit relates cycles (13: 79 mirror pairs among 160, 2 self-mirror; 15: 16 pairs in the 43-class); no map with |a| ≠ 1 or b ≠ 0; no equal consecutive differences (most common pairwise difference 6 times for 11+10i, 8 for 13); excess V - kn of 4 in 10 entries (13) and 26 in 33 (11+10i); 13's 160 cycles use 35 necklaces (multiplicities 2 to 14), 11+10i's 728 necklaces all distinct; composite classes: 18 of 29 66-cycles of 11+10i split as 257 + 257, the two 20-cycles of 13 as 73 + 75, the 75-cycles of 15 contain a 43-block of sum 336. Computed. FAM 4.

3.7.23. Angles: elements spread over all angles (10-degree bins: 2599 to 2715 for 11+10i's 33-cycles, 156 to 188 for 13's 10-cycles); share within relative distance 0.02 of an axis or diagonal 0.049, 0.054, 0.065, 0.051 against 0.051 uniform; 15 bunches near 0.066 (about 4 degrees), median absolute distance 352. Computed. FAM 2.

3.7.24. Lines and cycles: for real α a cycle meets L in all or none of its elements; 13: 6 of 160 10-cycles on L; 15: 2 of the 8-cycles and 2 of the 32-cycles; 11+10i: 37 of 728 have some elements on L by chance, only the 2-cycle through 1+2i lies on L; 11 at (1+i)^6: no 37-cycle meets L (6 or 19 of about 23,500 cycle elements, from three 2-cycles and for B, C one 13-cycle); 9: 4 or 6 of 15 or 17 6-cycles entirely on L. Computed. FAM 4, 6; D2 5.5; LOG families_lines.txt.

3.7.25. No parametrized family with a fixed number of odd steps can exist (each class is finite by Lemma 1); a family with growing n would need rel_n of order n|α|^(-1)/|z|; counts along multiples for 11+10i fall as predicted (764, 29, 2, 3, 0, 1, 1 for m = 1 to 8, the second figure the observed). Paper and computed. FAM 5.

3.7.26. Finiteness open: needs an effective lower bound for |ε(1+i)^V - α^n|/2^(V/2), a linear form in two logarithms (real α) or a logarithm and an angle; with a Baker-type bound, Lemma 1 would make each check finite (as Simons and de Weger after Steiner). Open. FAM 5; D2 5.6, 9 item 3.

3.7.27. Records: best(n) = min rel over V, ε; n is a record if best(n) < best(m) for all m < n; for real α the records are convergent denominators of log2|α| (plus an occasional small-n extra, n = 2 for 5, n = 3 for m = 19); for complex α the best simultaneous approximations. Definition. CONV preamble.

3.7.28. Largest class always at a record, and every class with ≥ 3 cycles except multiples and sums of the main record (74, 111, 148 for 11; 66, 132 for 11+10i; 75 for 15); dominant lengths for real α: 10 (13); 32, 43, 247, 537 (15); 37 and 2, 11, 13, 24 (11 at (1+i)^6); 6 (9); 3, 28 (5 at (1+i)^4); 5, 26 (7 at (1+i)^5). Computed. CONV 1, 4.1.

3.7.29. Records table up to n = 3000 (best, E, cycles): 13: 1 (0.19, 2.6, 1), 3 (0.073, 3.2, 2), 7 (0.065, 0.76, 0), 10 (0.0031, 117, 160), 227 (1.3·10^-4, 2·10^-10, 0); 15: 1, 10, 11, 32, 43, 247, 290 (0.0012, 0.72, 0), 537 (1.7·10^-4, 1.3, 2); 11+10i: 1, 2, 14, 19, 33; 15+2i: 1, 6, 1505; 9: 1, 5, 6, 47, 53, 153, 665; 11 at (1+i)^6: 1, 2, 11, 13, 24, 37, 949 (E 2.9), 986 (5.2), 1935 (2.0); 5 at (1+i)^4: 1, 2, 3 (29, 28), 28 (2.3, 3), 59 (1.2, 1), 146; 7 at (1+i)^5: 1, 5, 26, 109; 10+7i at (1+i)^7: 1, 4, 9, 616. Computed. CONV 4.1; LOG convergents_families.txt.

3.7.30. Continued fractions: log2 13 = [3; 1, 2, 2, 1, 22, 23, ...] (q = 1, 3, 7, 10, 227); log2 15 = [3; 1, 9, 1, 2, 1, 5, 1, ...] (1, 10, 11, 32, 43, 247, 290, 537); log2 11 = [3; 2, 5, 1, 1, 1, 25, ...] (1, 2, 11, 13, 24, 37, 949, 986); log2 9 (1, 5, 6, 47, 53, 153); log2 5 = [2; 3, 9, 2, 2, ...] (1, 3, 28, 59, 146); log2 7 (1, 5, 26, 109); ratios 37/10, 128/37, 168/43, 2098/537. Computed. CONV 4.1.

3.7.31. Prediction: 11 at (1+i)^6 should have a few cycles at the convergent lengths 949 and 986 (E = 2.9, 5.2), out of reach (Lemma 1 puts them near norm 10^13). Heuristic prediction, open. CONV 4.1, 7.

3.7.32. Census-wide: 54 rules (one per mirror pair, ≥ 3 cycle classes, (1+i)^4 to (1+i)^7) rerun at 10^6 (`convrun.py`, outputs in scratchpad only); 116 classes with n ≥ 2, 67 at records holding 1909 of 2104 cycles; in every class with ≥ 3 cycles (V, ε) is the best pair; examples n = 2 for 9+4i (4 or 5 per digit set) and 12+5i (7), n = 3 for 7+2i and 11+6i, n = 5 for 13+2i, n = 11 for 4+3i at (1+i)^4, n = 16 for 9+6i. Computed. CONV 3, 4.2; LOG convergents_census.txt.

3.7.33. The converse fails: records without cycles (13 at 7, 227; 9 at 5, 47, 53, 153; 15 at 290), mostly small E; only two records with E ≥ 1 and no cycle (n = 2 for 6+5i at (1+i)^5, E = 1.0; n = 16 for one digit set of 9+6i, E = 1.3), consistent with Poisson fluctuation. Computed. CONV 1, 6.

3.7.34. Sporadic cycles at non-record lengths: 8+7i at (1+i)^6, n = 82, rel 0.10, E = 0.005, 79% of the basin (others at 84, 129, 25); 5+2i at (1+i)^4 (the mirror digit set, i.e. 5-2i's 70-cycle), n = 70, rel 0.07, E = 0.018; 10+3i at (1+i)^6, n = 70, E = 0.009. Computed. CONV 6 item 2.

3.7.35. Simultaneous versus modulus-only: 9+4i (log2 97 = [6; 1, 1, 2, 458, ...]; n = 5 modulus error 0.0004, angle error 0.34, no 5-cycles; cycles at n = 2, angle error 0.065); 12+5i (n = 5 angle 0.49, none; 7 cycles at n = 2, angle 0.005); 13+2i (modulus records 7, 23 with angle 0.64, 0.53, none; full record 5 has 3); 11+10i (n = 33 both 0.0008 and 0.0007; modulus records 4, 5 bad angles, none). Computed. CONV 4.3; LOG convergents_modonly.txt.

3.7.36. The record property follows from E: if E rel² decreases in n the maximizing n is a record; the record notion is a corollary of the heuristic, not an independent principle. Heuristic argument. CONV 1, 7.

3.7.37. Integer analogue with digits ±1 at modulus 4 (m odd 3 to 31): only cycles besides ±1 are m = 5, n = 3 (±7, 7/3); 9, n = 6 (±13, 19/6); 11, n = 2 (±1, V = 7); 13, n = 3 (±1, V = 11) and n = 10 (±25, ±49, 37/10); 7 and 15 only ±1; none for m ≥ 19 (far outside the modulus-4 window). Computed to 2·10^5. CONV 5; LOG convergents_int_sym.txt.

3.7.38. Integer resonances (heuristic E with |U| = 2): m = 181 at 128 (n = 2, V = 15, 0.999786): 293 found, E 292.6, 294 cycles; 1449 at 1024 (2, 21): 53, 53.5; 1447: 41, 39.2; 645 at 512 (3, 28): 85, 84.5; 97 at 64 (5, 33): 49, 45.3; 1505 at 1024 (9, 95): 92, 89.9; 363 at 256 (2, 17): 12, 11.8; sweeps across √2 at 512 and 1024 within about 15% (e.g. 16 vs 14.4 at 1445, 4 vs 4.1 at 1437). Computed; E heuristic. IC 4.4; LOG zresonance.txt.

3.7.39. Prior art: convergents of log2 3 as constraints on 3x+1 cycle lengths (Crandall 1978, Steiner 1977, Eliahou 1993, Halbeisen-Hungerbühler 1997, Simons-de Weger 2004/2005; m-cycles ruled out for m ≤ 68; 3x+q with q = 2^r - 3^s; a 34-step cycle of 7x+11); Belaga and Mignotte on 3x+d (D = 2^l - 3^k gives many cycles); Cox-Ghosh-Sultanow, Gupta; arXiv:2005.00346 lists 181x+1 and 5x+1 cycles. The empirical version (cycles concentrate at convergent lengths with counts set by rel) was not found stated; may be in Lagarias 1990 or Belaga-Mignotte, not read. Literature. CONV 2, 7.

3.7.40. Superseded COL 5 item 8 suggestion: "many cycles of the same length suggests families of cycles rather than a finite list". Replaced by the arithmetic explanation and Theorem 1. COL 5 item 8; FAM 1.

3.7.41. Superseded COL 5 item 8 claim that many-cycle maps are those with λ = α/(1+i)^7 close to a real multiple of a unit (13, 15 on the diagonal; 11 ± 10i with λ = (1+21i)/16 nearly imaginary). Replaced by near-coincidences of α^n with ε(1+i)^V. COL 5 item 8; FAM 3.

## 4. Miscellaneous

### 4.1 The multiplier 3 and whether 3x+1 is a special case

4.1.1. On the real line G_3 is the accelerated 3x+1 map applied to whichever of ±x is 1 mod 4 (a non-real unit gives v(3x ± i) = 1, so u = ±1; dividing an integer m by (1+i)^(v(m)) gives ±m/2^(v2) or ±i m/2^(v2)). Paper. D1 6; D2 3.7; COL 2.

4.1.2. Since y ≡ 1 mod 4, 3y+1 is divisible by 4 and |S(x)| ≤ (3|x|+1)/4 < |x| for |x| > 1: every real orbit decreases to 1; the four other cycles lie off the axes. Paper. D1 6; D2 3.7.

4.1.3. 3 is inert in Z[i], so multiplication by 3 preserves the real line; split primes 2 ± i and 5 ± 2i leave no line invariant. Paper. D1 7; D2 3.7.

4.1.4. Proposition 1 (D2; Prop 5 in D1): for α = 3, (1) at modulus (1+i)^k with k ≥ 3 and integer digits on odd integers, |S(x)| < |x| on the axes once |x| exceeds the largest digit; (2) if the odd step at every positive odd n is 3n+1 then k ≤ 2 (10 = -i(1+i)²(2+i)(2-i)) and the map is outside the window (9 > 2^(k+1)). Part (2) in Lean (`prop5_2`, `prop5_2'`; the step to 9 > 2^(k+1) not formalized); part (1) paper. D1 Prop 5; D2 Prop 1.

4.1.5. No map in the family both agrees with 3x+1 on the positive integers and contracts on average in the plane. Paper. D1 7; D2 3.7.

4.1.6. Dividing by (1+i)² = 2i turns the real axis into the imaginary one; maps that do not commute with the units keep only the two axes; residue-class digits cannot see the sign of x. Paper. D1 7.

4.1.7. The multiplier 3 falls at (1+i)^3, where no map commuting with the units has a single cycle, with minimal or tried non-minimal digits; "the single cycle of 3x+1 does not survive at its natural Gaussian level". Computed. D1 Abstract, 5.1.

4.1.8. Open question: a map with class-dependent multipliers or moduli (Matthews type) restricting to 3x+1 on the real line with a single cycle; would need modulus at most (1+i)^2 on the class of 3 mod 4; untested; Conway's theorem means one at a time; a single-cycle example would include the Collatz conjecture. Open. D1 7, 13 item 6; D2 9 item 6; LAG 3 item 2.

4.1.9. Literature: no source on G_3, its five cycles, its axes, or Proposition 1; nearest relatives restrict to 3x+1 in another sense (Joseph on Z_2; Kohl's Z × Z permutation of Example 3.13). Literature negative. LIT 3.4, 2.8.

### 4.2 Real multipliers, divergence and the eight lines

4.2.1. Orbits that appear to diverge only for α = 5 at (1+i)^4 and 9, 11 at (1+i)^6, all real; none for non-real α. Computed. D2 5.5; D1 8.

4.2.2. α = 5: of about 1500 random starts with norm up to 1.8·10^7, 176 appeared to diverge, all on x - y = ±1 or x + y = ±1, ratio -5/4 per odd step; 876 of 1000 sample points of the diagonal lines map back onto them. Computed. D1 8; COL 3.7.

4.2.3. Modulus (1+i)^6: about 150 random starts with |x|, |y| ≤ 1000 per digit set; escaped past norm 2^120: α = 9: 16 of 152 (A), 13 of 142 (B), 16 of 147 (C), 29 of 160 (D); α = 11: 68 of 145, 69 of 156, 65 of 147, 55 of 147; all on the eight lines, all but 13 on diagonals, valuations 6, 7, 8 only. Computed. FAM 6; D2 5.5; LOG families_k6_escape.txt.

4.2.4. The eight lines L: x = ±1, y = ±1, x - y = ±1, x + y = ±1 are mapped into L by S for α = 3 ((1+i)^3), 5 ((1+i)^4, both digit sets), 7 ((1+i)^5), 9, 11 ((1+i)^6, all four), 13, 15 ((1+i)^7), with valuations k, k+1, k+2; argument: z = u + tw, transverse coordinate fixed by αu + d, which depends only on t mod a power of 2; checked over a window of 2^16 (and 2^18) consecutive t from 10^12 on two lines, the rest by units; bounded valuations show the window covers a full period. Computer-assisted (residue period check). FAM 6 Prop 3; D2 5.5.

4.2.5. At odd k, an odd valuation turns an axis line into a diagonal one and back; so with the axis lines included every point maps into L, replacing the 876 of 1000. Paper. FAM 6; D2 5.5.

4.2.6. Drift along L from a two-state chain (axis, diagonal), stationary distribution: 3 at (1+i)^3: mean v 3.750/3.500, share A 0.400, mean v 3.600, log2 N 3.170, drift -0.215 bits of |z| per odd step, converges; 5: 4.500/4.375, 0.200, 4.400, 4.644, +0.122, diverges; 7: 6.125/5.500, 0.444, 5.778, 5.615, -0.082, converges; 9: 6.500/6.188, 0.111, 6.222, 6.340, +0.059, diverges; 11: 6.500/6.563, 0.273, 6.545, 6.919, +0.187, diverges; 13: 7.938/7.500, 0.421, 7.684, 7.401, -0.142, converges; 15: 8.313/7.500, 0.471, 7.882, 7.814, -0.034, converges; independent of the digit set; sign matches observation in every case. Heuristic (equidistribution of t residues). FAM 6; D2 5.5; LOG families_line_drift.txt.

4.2.7. For α = 15 a uniform average over L would give +0.021, but the chain spends only 0.47 of its time on axis lines; exact iteration: 300 starts on L (t up to 2·10^9) all returned, mostly to the cycle through 7+8i; 200 starts on L for α = 9 all escaped (`famlinerun.py`). Computed. FAM 6; D2 5.5.

4.2.8. Superseded explanation: divergence occurs exactly when λ = α/(1+i)^k is real up to a unit (α real and k even: 5 at (1+i)^4 with λ = -5/4, 9 and 11 at (1+i)^6); "when λ is not real up to a unit the direction keeps turning"; α = 3 at (1+i)^3 and 7 at (1+i)^5 have λ² real and converge, "so the condition is on λ itself". Replaced by the invariance of L for all real α and the sign of the drift along L. D1 Abstract, 8, 11; COL 2, 3.7, 4, 5 item 8; FAM 6.

4.2.9. Superseded: COL 3.7 "on them the map acts like a one-dimensional 5x + c map ... drift ln 5 - 2 ln 2 > 0"; D1 8 "exactly four divisions per odd step". Replaced by the chain drift +0.122 bits with mean v 4.4. FAM 6.

4.2.10. The (1+i)^7 maps show no divergence although 13 and 15 are real: their drift along L is negative; (1+i)^7 = 8(1-i), so λ is never real up to a unit for odd α. Heuristic and paper. FAM 6; COL 5 item 8.

4.2.11. The lines carry the divergence, not the cycles (item 3.7.24). Computed. FAM 6; D2 5.5.

4.2.12. Precedents: Kionke's divergent cones (the line version), Matthews's Example 10.1 on Z[√2] (real multiplier 3, three invariant one-dimensional sets); the divergent maps are those whose map with exactly k divisions is a scalar map in Kionke's sense (old reading). Literature. D1 8, 11; D2 5.5; LIT 2.5, 2.6.

4.2.13. Every digit set tried with α = 5, 9, 11 had such orbits; at (1+i)^6 all sampled non-commuting maps with α = 9, 11 had escaping starts. Computed. D1 8; ST 5.3.

4.2.14. Open: prove divergence for α = 5, 9 or 11 by an exact description of the points on L; replace the equidistribution heuristic. Open. D1 13 item 2; D2 9 item 4.

4.2.15. Integers have no analogue of the lines (the line is the whole ring), and every integer map in the window has negative drift. Remark. IC 4.2.

4.2.16. Figure idea: an escaping α = 11 orbit plotted in the plane, settling onto a diagonal line of L. This inventory.

### 4.3 Joseph's map and divide-by-2 variants

4.3.1. Joseph's map on Z[i]: mean change of ln N per step (3/4) ln N(α) - ln 4, +0.26 for α = 3; contracts only for N(α) < 4^(4/3) = 6.35, i.e. N = 5. Heuristic. D1 10; D2 2; COL 2.

4.3.2. Of 314,197 starts with N(z) ≤ 10^5, 308,624 (98.2%) had not reached a cycle after 20,000 steps or passed coordinates 2^100; the rest fell into 67 cycles, not compared with Kucinski's 77 (an exhaustive count by period, so consistent). Computed to 10^5. D1 10; D2 2; LIT 2.2.

4.3.3. Caution: Kucinski's proof of Corollary 5 refers to "divergent" orbits; what she claims about them was not seen. Literature gap. LIT 3.5.

4.3.4. Variants: with α = 1+2i or 1-2i (drift -0.18) all 16 small-β choices converge on N(z) ≤ 10^5 and 8 have a single cycle (e.g. β = 1, i, -1+i, one cycle through 1 of 6 steps); with α = 2 ± i each has 3 or 4 cycles. Computed (10^5 in D1, "B = 10^6" in the COL heading). D1 10; COL 3.4, 4 item 3.

4.3.5. These variants are not in Kucinski's paper. Literature negative. LIT 2.2, 3.5.

### 4.4 The Eisenstein integers

4.4.1. Setting: z = a + bω, ω² = -1 - ω, N = a² - ab + b²; P = (2) inert, q = 4; units ±1, ±ω, ±ω²; canonical representative 0 ≤ b < a; window (4^k, 4^(k+1/3)); modulus 2 has no multiplier ((4, 6.35) holds no norm). Paper. IC 7; ST 10.

4.4.2. Modulus 4: 12 odd classes in 2 orbits; unique minimal set U ∪ (1-ω)U; window (16, 25.40) holds norms 19, 21, 25: 5+2ω and 5+3ω (19), 5+ω and 5+4ω (21), 5 (25). Paper and computed. IC 7.

4.4.3. Results at N(z) ≤ 10^5: 5+2ω (and conjugate): drift -0.290, 1 cycle (fixed class of 1); 5+ω: -0.190, 3 cycles (1, 2+ω fixed, 7+3ω of 11 odd steps); 5: -0.016, 84 cycles found and 9104 unresolved starts. Computed to 10^5. IC 7; LOG zeis_k2_B1e5.txt.

4.4.4. 5+2ω and 5+3ω: every odd z with 1 < N(z) ≤ 10^6 reaches a smaller norm (exact, one per associate class; log max 96), so every odd z with N(z) ≤ 10^6 reaches a unit. Computed to 10^6. IC 7; LOG zeis_k2_drop.txt.

4.4.5. Lemma 3 accounts for the fixed classes: 4 - (5+ω) = ω² a unit; 4 - 5 = -1; 4 - (5+2ω) = -1-2ω of norm 3 in D, so only 1 is fixed. Paper. IC 7.

4.4.6. Real multiplier 5: of 139 random starts with |a|, |b| ≤ 300, 59 passed norm 2^400, all on b = 1 (33) or a - b = 1 (26). Computed. IC 7; LOG zeis_lines.txt.

4.4.7. Modulus 8: 48 odd classes in 8 orbits; one orbit tied (2-3ω, 2+5ω, norm 19), so two conjugate minimal sets; window (64, 101.6), 16 multipliers, norms 67, 73, 75, 79, 81, 91, 93, 97, 32 maps; none single-cycle at 10^5: mostly 2 or 3 cycles, 4 for two, 8 for 10+5ω, 10 or 14 for norm 73, α = 9: 42 cycles and 5829 unresolved; every map has a cycle through 1 (for 11+5ω, 11+6ω with one digit set a 2-cycle through 1). Computed to 10^5. IC 7; LOG zeis_k3_B1e5.txt.

4.4.8. α = 9 at modulus 8: 45 of 139 starts passed 2^400, all on b = 1 or a - b = 1. Computed. IC 7; LOG zeis_lines.txt.

4.4.9. So in Z[ω] the first modulus commuting with the units already has a single-cycle map, the smallest multiplier there, 5+2ω (with mirror), up to 10^6. Computed. IC 1 item 7, 7.

4.4.10. Literal check of all six associates: 12 of 30 maps pass (all associates of 5+2ω and 5+3ω); trivial cycles: one through all six units (-2-5ω, 2-3ω, 3+5ω, 5+3ω), two of three units (-5-3ω, -3-5ω, -2+3ω, 2+5ω), three of two units (-5-2ω, -3+2ω), six fixed units (3-2ω, 5+2ω); 8 of the 12 have trivial cycles that move between units; counts 1, 2, 3, 6 as Theorem 2 predicts. Computed to 10^5. ST 10; LOG eistest_k2_min.txt.

4.4.11. Modulus 8 literal: 64 minimal sets, 2 commuting; sample of 2 random tie-breaks for each of 96 associates (192 maps): 18 pass, all norm 91 (associates of 11+5ω, 11+6ω), 14 with moving trivial cycles. Computed to 10^5. ST 10; LOG eistest_k3_min_sample.txt.

4.4.12. Modulus 4 non-minimal: of 40 random digit sets (digits norm ≤ 13), 12 pass (norms 19, 21), 9 with a trivial cycle through two or more units. Computed. ST 10; LOG eistest_k2_nonmin.txt.

4.4.13. The REU slides mention an Eisenstein analogue dividing by 1-ω (the ramified prime, q = 3), without computations; our Eisenstein maps divide by the inert 2. Literature versus computed; the 1-ω version is untested. LIT 2.3; IC 7. This inventory notes the difference.

### 4.5 Other moduli without the criterion

4.5.1. Non-commuting sweeps at (1+i)^3 (α associates of 3, 3 ± 2i), digits norm ≤ 5: 972 maps, all converge, 108 single nonzero cycle. Computed to 10^5. D1 5.1; D2 3.2; COL 3.2.

4.5.2. (1+i)^6 sample: 6 random non-commuting digit-set orbits per associate, 672 of 7840 maps (112 × 70), cap 50000: 146 pass; by norm 65: 38/96, 73: 34/48, 81: 0/24, 85: 19/96, 89: 0/48, 97: 0/48, 101: 26/48, 109: 23/48, 113: 0/48, 117: 6/48, 121: 0/24, 125: 0/96 (inconclusive, 90 of 96 past gcz's 2^100); passing norm-65 maps all associates of 7 ± 4i; associates of 8 ± i cannot pass. Computed to 10^5. ST 5.3; LOG symtest_k6_sample_B1e5.txt.

### 4.6 Density, stopping times and related theory

4.6.1. Open: a Terras-Everett density theorem in (1+i)-adic form (parity pattern of the first n steps depends only on z mod a power of 1+i), expected to give finite stopping time on a set of density one for N(α) < 2^(k+1); not worked out; would not rule out divergence (Kionke). Open. D1 13 item 3; D2 9 item 5; COL 4 (b).

4.6.2. Nichols (Exp. Math. 2016, arXiv:1610.02545): a Terras-type theorem in function rings of curves over F_2 (abstract only). Literature. LIT 2.10; D2 9.

4.6.3. Siegel and O'Dwyer (arXiv:2609.33018, 26 Sept 2026): Tao-type almost bounded orbits on a set of logarithmic density one for p-Hydra maps on O_K under MH1 to MH4 and (p-1)²ρ^d < 1; H is not a 2-Hydra over 1+i; a naive encoding mod (1+i)^4 (p = 16) appears to fail the constraint by a wide margin. Literature plus rough estimate, unverified. LIT 2.9; D2 9 item 5.

4.6.4. Polynomial analogues: Hicks-Mullen-Yucas-Zavislak (2008), Matthews-Leigh (1987), Gallardo-Rahavandrainy (arXiv:2510.07530, every sequence finite), Monks (arXiv:2508.10153, q-analogs conjugate to 3x+1 on Z_2), Behajaina-Paran (arXiv:2312.00390; its map commutes with every constant, not discussed there); in positive characteristic all orbits are eventually periodic. Literature. LIT 2.10; COL 1; SLIT 3.5.

4.6.5. Garg (Zenodo doi:10.5281/zenodo.19078383, 18 March 2026): "four residue classes modulo (1+i)" (sic), affine rule per class, Variant E divides by 4, period-40 orbit in Z[1/2][i], box dimension about 1.70, negative Lyapunov exponents for Variant C; not integral on Z[i]; summary only, keep [unverified]. Literature. LIT 2.4; D2 2.

4.6.6. Aliyev (NNTDM 31(3) (2025) 658-666): rational cycles of (p_i x + k_i)/q, lists a Gaussian version as future work. Literature. COL 1; D2 2.

4.6.7. Complex-analytic interpolations of 3x+1 (Letherman-Schleicher-Wood 1999, Dumont-Reiter 2001, Pe 2004, Chamberland 1996, Berg-Meinardus 1994, 1995): not integral maps. Literature. LIT 2.11; COL 1.

4.6.8. Wirsching's book (LNM 1681, 1998): not seen; apparently Z and 3-adic only. Literature gap. LIT 2.12.

4.6.9. Lagarias 2016's other problems (bounded multiplicities of the Klarner-Rado sequence; Klarner's six orbits of 0; freeness of 2x2 integer affine semigroups "very challenging"; the rational part of the inverse orbit). Literature, background only. LAG 1.4.

### 4.7 Corrections and process notes not tied to one result

4.7.1. Proposition numbering changed between drafts: D1 Prop 1, 2, 3, 4, 5 = D2 Prop 2, 3, 4, 5, 1; Lean names `prop1`, `prop3`, `prop5_2` follow D1; TH has its own numbering (Thm 2.1, 3.1, 4.4, 4.7, 5.4, 6.1); ST's Theorem 4 is TH Thm 5.4; IC Theorem 1 and FAM Lemma 1 are separate. Process note. D2 8.

4.7.2. D2 8 says Propositions 1(1), 3 and 5 and the census are not formalized; the Lean table in D2 predates TH and lacks the Dynamics, Expr, Variants, Window and TwoMinusI files. Superseded table. D2 8; LEAN.

4.7.3. LIT 4 list of draft-1 changes (cite Kucinski, REU, correct Tremblay attribution, Joseph pages, Kionke journal, attribute the window to Matthews-Watts, Möller, Heppner, mention Matthews O_K, Kohl, Kionke's matrix extension, cite Example 10.1, Siegel-O'Dwyer and Nichols, keep [unverified] on Garg, soften the novelty claim); most were applied in D2. Process note. LIT 4.

4.7.4. LIT 1 access limits: shell proxy refused outside hosts, WebFetch truncates PDFs at 80,000 to 86,000 characters; Matthews's 3x+1 index page unreachable; a Google Docs PDF possibly the REU paper unreachable. Process note. LIT 1; SLIT 1.

4.7.5. "Collatz-type" criterion stated by the author (every nonzero orbit reaches a trivial cycle; cycles taken literally for maps that do not commute with the units). Definition source for the new vocabulary. ST 1.

4.7.6. gcz.c and gstop.c accept MOD 16 for (1+i)^7; gcz reports escapes separately from cap hits; sweep_eq2.py options --setup, --only, --cap; MAXC 4096 in gcz (65536 in the scratchpad copy). Code note. COL 5 item 2; FAM 2.

## 5. Items that belong to verification, open questions, code and references

5.1. Verification: gcz (memoized full classification, Brent, 128-bit, escape 2^100, tables mod 4, 8, 16); gstop (one associate per class, a > 0, b ≥ 0, exact comparisons below 10^36, escape 2^110, cannot report a false drop); gstopall (all quadrants); exact.py (unbounded, digit from the congruence); zcz (integers, multi-word above 2^100, escape 2^2000); zany; zverify.py; famverify.py; Lean 4 toolchain v4.35.0, core only, no sorry, admit, native_decide or bv_decide, `decide` checked by the kernel, tactics omega, decide, simp, grind, induction, rcases; build about 12 s; CheckAxioms prints 66 theorems (`prop3_int` to `E2_sound`), `pi_pow_four` and `digitH_unit_table` with no axioms. D2 7, 8; LEAN; TH 8.

5.2. Lean files: Basic, Props, Soundness, RuleH, HSound, Dynamics, Expr, Variants, Window, TwoMinusI; in Lean S is 0 on even inputs, cycles via `Reaches`, the count of trivial cycles via the three cases of `trivial_cycle_count`. LEAN; TH 8.

5.3. Open questions to collect: 3.5.11, 3.5.26 to 3.5.28, the single-cycle conjecture for the 22 maps at (1+i)^6 and 8 at (1+i)^7 (D2 9 item 2), whether single-cycle maps stay a minority at (1+i)^8 (window (256, 512)) (D2 9 item 3), 3.7.26, 3.7.31, 4.1.8, 4.2.14, 4.6.1, 4.6.3, the OEIS search for 5x ± 1 (2.2.9), the 1-ω Eisenstein version (4.4.13), the score (1.3.11), A305057 (1.1.26).

5.4. Code and data: rule files `alpha5p2i_k4.rule`, `G3_k3.rule`, `G3p2i_k3.rule`, `k7_a-11_4.rule`, `k7_a-12_3.rule`, `k7_a-11_8.rule`, `k7_a-12_7.rule`; Python `rules.py`, `exact.py`, `sweep_eq.py`, `sweep_eq2.py`, `sweep_gen.py`, `joseph.py`, `bigrun.py`, `run_stop.sh`, `zsym.py`, `below_window.py`, the `fam*.py` scripts, `convergents.py`, `convrun.py`, `zcensus.py`, `zsummary.py`, `zverify.py`, `zesc.py`, `zresonance.py`, `zeis.py`, `zeisline.py`, `symtest.py`, `eistest.py`, `variants29.py`, `mirror29.py`, `entry2mi.py`; C `gcz.c`, `gstop.c`, `gstopall.c`, `zcz.c`, `zany.c`; logs as cited; `lupei.py`, `gczbig` and the convrun outputs live only in a scratchpad and are not reproducible from the repository as is. D2 10; IC 9; ST 12; FAM 8; CONV 8.

5.5. References to add or fix: Barina (arXiv:1807.00908; CMST 28(4) 2022), OEIS A317640, A317753, A318489, A318972, A006368, A037084, A305057; Tremblay arXiv:1907.13302; Carnielli 2015; Lagarias 2016 (Monthly 123(8)); Conway 1972 and 2013; Coxeter 1971; Collatz 1986; Klarner-Rado 1974; Klarner-Birget-Satterfield 1991; Cassaigne-Harju-Karhumäki 1999; Coppersmith 1975; Crandall 1978; Eliahou 1993; Halbeisen-Hungerbühler 1997; Belaga-Mignotte; Heppner 1978; Hasse 1975; Leigh 1985; Kucinski 2000; Chamberland; Siegel-O'Dwyer 2026; Nichols 2016; Kionke journal details to check; references from memory in D2 (17 to 23, 25) unchecked. LIT; LAG 3; SLIT; CONV 2; D2 References.

## 6. Suggested changes to the outline

6.1. Add 3.8 "The multiplier 3" for items 4.1.1 to 4.1.9 (G_3 on the axes, inert 3, Proposition 1, the open question on class-dependent maps). It sits naturally after the census entry for (1+i)^3 and answers the introduction's question of how 3x+1 fits.

6.2. Add 3.9 "Real multipliers: the eight lines and divergence" for items 4.2.1 to 4.2.16, including the superseded λ explanation as a remark. It belongs in section 3 because every divergent case commutes with the units.

6.3. Add 2.7 "Other moduli and non-minimal digits without the criterion" for items 4.5.1 and 4.5.2, together with 2.5.9 and 2.5.10, so that 2.5 can stay on the minimal maps at (1+i)^4.

6.4. Add a short section or appendix "The next ring: Eisenstein integers" for items 4.4.1 to 4.4.13, with the general window 2.1.7 and the thresholds of 3.2.3; the introduction's "next ring left open" then points to it.

6.5. Fold items 4.3.1 to 4.3.5 (Joseph's map on Z[i] and its divide-by-2 variants) into 1.1 as a short paragraph after Kucinski, since they are computations on a literature map rather than results about the family.

6.6. Move items 4.6.1 to 4.6.9 to the open questions and related-work part of the closing sections; none of them is a result of the paper.
