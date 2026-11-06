# Collatz results: the standing catalogue

Compiled 6 November 2026. This is the collection of what we have observed, demonstrated or proved about Collatz maps on N, Z, Z[i], the Eisenstein integers Z[ω] and general rings of integers, together with the literature results the papers draw on. It is not a paper. Papers take from it only what we decide to include. It replaces `collatz/papers/collatz-inventory.md` (403 items mapped to an old outline), and adds the results of the files that postdate it. Goldbach and Weilert work are kept elsewhere.

## 0. How to read an entry

Each entry has a short id, the statement, its status, prior art and credit, the source, and where it currently appears in the two drafts.

Status labels. Lean: proved in Lean 4 (core, toolchain v4.35.0, axioms at most `propext`, `Classical.choice`, `Quot.sound`), with the theorem names. Paper: proved on paper, not formalized, with the location of the proof. Computer-assisted: a proof that rests on stated program runs. Computed to X: an observation checked for every start up to the bound X. Heuristic. Literature: taken from a cited source. Open. Superseded entries are in Section 17.

Source key. Files in `collatz/research/`: TH `theorems.md`, M3 `modulus3-search.md`, IAP `invariants-prior-art.md`, LEI `leigh1986.md`, MW `matthews-watts1985.md`, RPA `rotation-prior-art.md`, SC `score.md`, ST `symmetry-test.md`, IC `integer-census.md`, FAM `families.md`, CONV `convergents.md`, COL `collatz.md`, LIT `literature.md`, SLIT `symmetric-literature.md`, LAG `lagarias2016.md`, LEAN `lean/` (files in `lean/GaussianCollatz/`), LOG `logs/`. Older drafts: D1 `collatz/papers/gaussian-collatz.md`, D2 `collatz/papers/gaussian-collatz-2.md`. INV is the superseded inventory, cited only where a figure was computed for it and is written up nowhere else.

Paper key. P1 is `collatz/papers/collatz-rings.md` (Invariants of Collatz maps across rings of integers), P2 is `collatz/papers/collatz-5p2i.md` (The 5+2i problem). "P1 4.2" is a section, "P1 Prop 6.2" a numbered result. "None" means the result is in neither draft.

Vocabulary. A map is (α, k, d): multiplier α, modulus π^k, digit table d. A Collatz map is a map in which every nonzero orbit reaches a trivial cycle, a cycle that contains a unit; a unit cycle consists only of units. A map commutes with the units if T(uz) = uT(z) for every unit u. Rotated and mirrored maps are defined in Section 6. The window is q^k < N(α) < q^(k+1/(q-1)), which for q = 2 is 2^k < N(α) < 2^(k+1). The drift is the mean change of log2 N per odd step under the equidistribution assumption. The score is 1/M - 1/N with M = N(π^k) and N = N(α). Older names ("level", "expression", "rule", "symmetric", "unit-equivariant", "Collatz-like", "minimal" as the ordering principle) are retired; Lean identifiers keep some of them (`Expr`, `Expr.IsRule`, `Symmetric`).

Throughout π = 1+i in Z[i], U is the unit group, and for Z[i], π^3 = -2+2i, π^4 = -4, π^5 = -4-4i.

## 1. Framework and definitions

### 1.1 Definitions and basic lemmas

F1. In Z[i], a+bi is even (divisible by π) exactly when a+b is even; (a+bi)/(1+i) = ((a+b)+(b-a)i)/2; dividing by π halves the norm; since 2 = -iπ^2, the exponent of π in an integer is twice its exponent of 2; conjugation preserves parity and π^k divides x iff it divides x̄. Status: paper. Source: TH 1; D2 3.1. Papers: P1 4.1; P2 1.2.

F2. Definition of a map (α, k, d) over R: α odd, k ≥ 0, d a function on odd z depending only on z mod π^k with π^k | αz + d(z); digit set D; T(z) = z/π on even z and αz + d(z) on odd z; accelerated S(z) = w/π^v(w) with w = αz + d(z), S(z) = 0 if w = 0. Status: definition; for Z[i] in Lean (`Expr`, `Expr.T`, `Expr.S`; in Lean S is 0 on even inputs). Source: TH Def 1.1. Papers: P1 Def 2.1; P2 1.2.

F3. Lemma: π | xy implies π | x or π | y; for odd α, π^k | αx implies π^k | x; for k ≥ 1 every digit is odd and D has exactly one element in each odd class, so d(z) is the element of D in the class of -αz and a map is a choice of one odd element per odd class. Status: Lean for Z[i] (`pi_dvd_mul`, `pow_dvd_of_odd_mul`, `Expr.d_odd`, `Expr.d_inj`); paper for general R (P1 Lemma 2.2). Source: TH Lemma 1.2. Papers: P1 Lemma 2.2; P2 1.2.

F4. The congruence d(z) ≡ -αz (mod π^k) depends only on the class of z, and z ↦ -αz permutes the q^(k-1)(q-1) odd classes. Status: paper. Source: D1 2; D2 3.1. Papers: P1 2.1.

F5. Definitions of periodic point, cycle, trivial cycle, unit cycle, trivial map and Collatz map; cycles taken literally as sets. Status: definition; Lean for Z[i] (`Periodic`, `Reaches`, `OnTrivialCycle`, `Trivial`, `Expr.IsRule`). Source: TH Def 1.3; ST 1 (the author's criterion: every nonzero orbit reaches a trivial cycle, cycles literal for non-commuting maps). Papers: P1 Def 2.3; P2 1.2.

F6. Two cycles that meet are equal: if x is periodic with period p and y = f^j(x), then f^(j(p-1))(y) = x. Status: Lean (`reaches_back`). Source: TH Lemma 1.4(a). Papers: P1 3.2, Prop 6.2 proof.

F7. Triviality can be read off T or S alike: a nonzero even element reaches its odd part under T, only even elements lie between consecutive odd terms, and units are odd. Status: paper. Source: TH Remark 1.5. Papers: P1 2.1; P2 1.2.

F8. A map commutes with the units iff d(uz) = u d(z) iff D is closed under multiplication by units (k ≥ 1); then S(uz) = uS(z) and S^n(uz) = uS^n(z). Status: Lean (`Expr.Symmetric`, `Expr.S_commutes`, `iter_unit_mul`). Source: TH Lemma 1.4(b), (c). Papers: P1 1.3; P2 1.2, 3.1.

F9. For every map, z ↦ uz conjugates (α, D) to (α, u^(-1)D) (digit table z ↦ u^(-1)d(uz)); the four unit images of a digit set always give identical cycle counts and basins. Status: paper; computed agreement in every run (ST 2; M3 3 item 3). Credit: the REU associates proposition at modulus 1+i, and on Z the passage from 3x+1 on the negatives to 3x-1, are this conjugacy (RPA 3.1, 3.9). Papers: P1 4.2, 6.5; P2 1.2, 2.1.

F10. A map that commutes with the units acts on classes of associates: every odd Gaussian integer is a unit times one with a > 0, b ≥ 0, so one representative per class suffices for a check and cycles are counted up to units; a conjecture of Collatz type for such a map speaks about all of Z[i] \ {0}, with no "half" as for 3x+1 on Z. Status: paper; Lean (`exists_unit_primary`). Source: D1 5.1; D2 1. Papers: P2 3.2.

F11. An f-invariant set of non-units containing an odd element rules out triviality. Status: Lean (`not_trivial_of_invariant`). Source: LEAN Dynamics.lean. Papers: used in the proofs of P1 Prop 4.1 and P2 Thm 4, not stated.

F12. Invariants of the rings used: N (units 1, π = 2, q = 2), Z (±1, 2, 2, e = 1, κ = 1, line), Z[i] (four units, 1+i, q = 2, ramified, e = 2, κ = 2, square lattice, conjugation moves π: π̄ = -iπ), Z[ω] (six units, 2, q = 4, inert, e = 1, κ = 1, triangular lattice, conjugation fixes π). Status: paper. Source: IC 2.3, 7. Papers: P1 1.3 table.

### 1.2 Background from the literature

F13. History: Collatz says he invented the problem before 1952 and passed it to Hasse in 1952; Coxeter gave the first published statement in 1971 with prizes of 50 and 100 dollars; Erdős published nothing on 3x+1, called it "hopeless" in conversation, and the "mathematics is not ready" remark is not in Lagarias's account; Erdős's only prize in Lagarias 2016 concerns the density of ⟨2x+1, 3x+1, 6x+1 : 1⟩. Status: literature (Lagarias 2016, p. 772; Collatz 1986; Coxeter 1971). Source: LAG 1.2, 1.5, 4. Papers: P1 8 item 1.

F14. Lagarias 2016 restates 3x+1 through the affine semigroup ⟨2x, (2x-1)/3⟩: the integers in the orbit of 4 form the backward tree, a non-integer never returns, and the orbit is not discrete, so Erdős's density bound does not apply; the x+1 problem, with free inverse semigroup ⟨2x, 2x-1⟩, is the "solved cousin". Klarner and Rado introduced orbits ⟨R : A⟩ (Pacific J. Math. 53 (1974)). Status: literature. Source: LAG 1.3, 1.5, 2. Papers: P1 2.2, 8 item 13; P2 4.4.

F15. Conway (1972) proved a generalization of 3x+1 undecidable, so maps of this kind are studied one at a time; Conway 2013 on unsettleable problems. Status: literature (via Lagarias 2016). Source: LAG 1.5. Papers: P1 8 item 2.

F16. Frameworks that contain our maps: Kohl's RCWA (rcwa maps of Z^2 with 2×2 matrix coefficients include each step of a Gaussian map; RCWA itself implements only Z, Z^2, semilocalizations Z_(π) and GF(q)[x]); Kionke's lattice maps (tame cones give divergent orbits of positive density; on Z[√2] at least half diverge; integer matrix multipliers proposed as an open extension); Siegel's Hydra maps on O_K (no Z[i] example); Siegel and O'Dwyer (almost bounded orbits on a set of logarithmic density one under (p-1)^2 ρ_H^d < 1, no Z[i] or Z[ω] example). Status: literature, partly unverified (RCWA manual chapter 2, Siegel and O'Dwyer's MH1 not read). Source: LIT 2.5, 2.8, 2.9; IAP 2.4. Papers: P1 1.1, 8 items 10, 11.

F17. Literature search result: as far as the search reached, no earlier work takes the ring as a parameter, studies maps on Z[i] dividing by 1+i at moduli (1+i)^k with k ≥ 2, or uses the criterion of commuting with the units. Gaps: Lagarias's bibliography I from M on, Garg's formulas, the REU paper, Matthews's web pages, Hasse, Möller, Wirsching, Venturini, most of The Ultimate Challenge. Status: literature negative. Source: LIT 3; SLIT 5; IAP 1, 4. Papers: P1 1.1.

F18. Works checked and found to concern Z only or to contain no map commuting with -1 (Tremblay, Carnielli, Goodwin, Hohertz-Kalantari, Rozier, Gilbert, Bruschi, Rorabaugh, Edgington, Briscese-Calogero, Goyal, Shrimali-Antu-Jones, Barina-Maat, Bouhamidi, Ye, Siegel 1909.09733, Behajaina-Paran, Diarte-Carot, Hamada, Lesieutre, Karami and others). Status: literature negative. Source: LIT 2.13; SLIT 3.5; IAP 4; RPA 3.11. Papers: none.

## 2. The window and drift

W1. Below the window: if N(α) < q^k, every orbit of T is eventually periodic (ρ = |α|/|π|^k < 1, every orbit enters the disc |z| ≤ R_0 = δ/(|π|^k(1-ρ)) and stays). Status: paper, not in Lean. Credit: on Z it follows from Leigh's Lemma 3, (61), unconditionally (LEI 3.1); MW 1985 does not contain it (MW 3.3). These maps play the part of the x+1 problem. Source: D2 Prop 3; IAP 3.1. Papers: P1 Prop 2.4; P2 7.2 (listed as not formalized).

W2. Worst case above the lower end: an odd step with exactly k divisions multiplies the norm by N(α)/q^k > 1. Status: paper. Source: D2 3.4. Papers: P1 2.2; P2 1.2.

W3. In density over odd z (Haar measure on the π-adics), v - k is geometric with P(v - k ≥ j) = q^(-j), mean 1/(q-1); the mean change of log2 N per odd step is log2 N(α) - (k + 1/(q-1)) log2 q, negative exactly inside the window. For q = 2: mean 1, variance 2. Status: paper for the density statement, heuristic for orbits. Source: IC 2.4; D2 3.4. Papers: P1 2.2; P2 4.3.

W4. Lemma: write the map at modulus π^k as T' with z ↦ z/π on the q^(k-1) even classes and z ↦ (αz + d)/π^k on the odd ones; its Markov matrix on classes mod π^k (fractions of classes mod π^(2k)) is doubly stochastic and irreducible, so the stationary weights are uniform, q^(-k); the weighted product of norm ratios is (1/q)^(1/q)(N(α)/q^k)^((q-1)/q), below 1 exactly when N(α) < q^(k+1/(q-1)). Status: paper (P1 Lemma 2.5 for general R); the Z argument is in MW 3.2, and Q was built and found doubly stochastic for Z[i] at (1+i)^4 with α = 5+2i, 3, 1+4i, 5+3i (MW 3.4, scratchpad scripts). Credit: MW 1985 Lemma 3.2 gives doubly stochastic Q only when every m_i is prime to the modulus, which fails on even classes. Source: MW 3.2, 3.4. Papers: P1 Lemma 2.5, 2.2.

W5. The window, Definition: q^k < N(α) < q^(k+1/(q-1)); for q = 2, 2^k < N(α) < 2^(k+1); for 3x+1, 2 < 3 < 4 at modulus 2; for 7x ± 1, 4 < 7 < 8 at modulus 4. Prior art: on Z the upper end at every k is the dividing line of Matthews and Watts 1985, Conjectures 1 and 2 (p. 30), which apply because the encoded map satisfies their condition C with β = 1 and m = d (all trajectories eventually periodic below, almost all divergent above; positive multipliers only); Leigh 1986, Conjecture 1, Corollary (iii), (4), is the one-sided form for nonzero multipliers with |m_i/d|; at k = 1 the upper end is Heppner's threshold d^(d/(d-1)) for Hasse's maps (Arch. Math. 31 (1978) 317-320, via Lagarias bibliography I entry 78; paper not seen), with Hasse's case m = d+1 inside; Crandall's conjecture that qx+1 diverges for q ≥ 5 is the case q = 2, k = 1; Siegel and O'Dwyer's heuristic ρ_H < 1 gives the same upper end if the operator norm of multiplication by α is |α| (our computation). The norm form and the exponent 1/(q-1) were not found in print. Status: definition, with the literature identification. Source: IAP 2.1, 3.1; LEI 3.1; MW 3.1 to 3.4, 4.1. Papers: P1 Def 2.6, 2.2, 6.7; P2 1.2 (q = 2).

W6. With the unaccelerated encoding (m_i = 2^(k-1) on even classes, 2^k m on odd ones) the weights are not uniform (odd classes total 1/(k+2)); the product is (m/2^(k+1))^(1/(k+2)) with the same threshold, while the unweighted product gives the wrong answer (1.22 > 1 for 3x+1). So the weighting of MW 1985 makes the criterion independent of how the map is written. Status: computed exactly, with argument. Source: MW 3.2. Papers: none.

W7. Weighted products, first and second encoding: 3x+1 (k = 1) 0.866, 0.909; 5x+1 1.118, 1.077; 7x ± 1 (k = 2) 0.935, 0.967; 9x ± 1 1.061, 1.030; 21x+d (k = 4) 0.810, 0.932; 29x+d 0.952, 0.984; 31x+d 0.984, 0.995; 33x+d 1.016, 1.005; in every case the eigenvalue 1 of Q(d) is simple. Status: computed exactly. Source: MW 3.2. Papers: P1 2.2 (0.866, 0.935).

W8. MW 1985 Theorem 1.1(b), applied with the first encoding, gives the drift per odd step, log2(m/2^k) - 1; their Lemma 2.1 shows the digits drop out of Q(m) when gcd(m_i, m) = 1, so for our maps the weights do not depend on the digit table (also visible in Leigh, whose chain depends only on the m_i and d). Status: literature plus derivation. Source: MW 3.3, 4.6; LEI 3.6. Papers: none.

W9. "Divergent" in MW and Leigh means "not eventually periodic", which equals "unbounded" only when there are finitely many elements of bounded size: true for Z, Z[i], Z[ω], false for Z[√2] with the absolute value; Leigh's Lemma 3 and our W1 both need it. Status: remark. Source: LEI 3.1; MW 3.4. Papers: P1 5.2 (Z[√2]).

W10. The windows for different k are disjoint, so in one ring the norm of a multiplier in the window determines its modulus, and ordering by modulus then norm is ordering by norm. Status: paper. Source: D2 3.5. Papers: P1 2.2.

W11. Which moduli carry multipliers: π^k does iff (q^k, q^(k+1/(q-1))) contains the norm of an odd element. In Z for every k ≥ 1; in Z[i] exactly for k ≥ 2 (odd norms are 1 mod 4; witnesses 2^j + i for k = 2j, (2^j+1) + 2^j i for k = 2j+1); in Z[ω] exactly for k ≥ 2 (modulus 2 needs 5, not a norm; witness 2^k - ω of norm 4^k + 2^k + 1). Status: paper. The general-k proof is only in P1; the research files give the cases tabulated up to (1+i)^7 (W12) and Eisenstein moduli 2, 4, 8 (IC 7). Credit: not found as a principle; the REU slides and an uncitable blog post touch it (IAP 3.2). Papers: P1 Prop 6.1.

W12. Odd α in Z[i] has N(α) ≡ 1 (mod 4). Multipliers in the window up to units: moduli 1 and 1+i none; (1+i)^2: norm 5 (2 ± i); (1+i)^3: 9, 13 (3, 3 ± 2i); (1+i)^4: 17, 25, 29 (4 ± i, 5, 4 ± 3i, 5 ± 2i); (1+i)^5: 37, 41, 45, 49, 53, 61 (11 multipliers); (1+i)^6: 65 to 125, twelve norms (28 multipliers); (1+i)^7: 137, 145, 149, 153, 157, 169, 173, 181, 185, 193, 197, 205, 221, 225, 229, 233, 241, 245 (46 multipliers). Status: computed exactly. Source: D2 4.4; COL 2, 5 item 1; TH Lemma 5.3. Papers: P2 2 (to (1+i)^4), 5.3 (18 norms, 46 multipliers).

W13. In Z[ω] the naive window (4^k, 4^(k+1)) would admit 23 multipliers up to units at modulus 4 instead of the five in (16, 25.40); all but those five expand on average. Status: computed. Source: IC 2.4. Papers: P1 5.1.

W14. Drift definition: log2(N/M) - log2 q/(q-1) bits per odd step, which for q = 2 is log2 N(α) - (k+1); for 3x+1 on N, -0.415; for 7x ± 1, -0.193; for the 5+2i map, -0.142 (= ln(29/32) = -0.098 in natural log). Inside the window the drift lies in (-log2 q/(q-1), 0). Status: definition and arithmetic. Source: SC 1; D2 3.4, 6.4; IC 5. Papers: P1 Def 2.7, 2.3; P2 1.2.

W15. Score definition 1/M - 1/N = (N-M)/(NM); inside the window it lies in (0, 1/M - q^(-k-1/(q-1))), for q = 2 in (0, 1/(2M)); below the window it is negative; within one modulus score and drift order maps alike, across moduli the score favours the smaller modulus; compare within one ring and one class. Prior art for such a score was not searched. Status: definition. Source: SC 1, 2 (q = 2); the general-q range only in P1 2.3. Papers: P1 Def 2.7, 2.3; P2 1.2, 5.4.

## 3. Trivial and unit cycles

T1. Any map has at most |U| trivial cycles (distinct cycles are disjoint and each trivial cycle contains its own unit). Status: Lean for Z[i] (`trivial_cycles_le_four`, `trivial_cycle_has_unit`, `Expr.trivial_cycles_le_four`); paper for general R. Credit: not found (IAP row 3); implicit on Z in every list of the cycles of 3x+1, and in Barina's remark that 7x ± 1 has the cycles through 1 and -1 only. Source: TH Thm 2.1(a); ST Thm 2(1). Papers: P1 Prop 6.2(1), 3.2; P2 2.

T2. If f(uz) = uf(z) for all units: if 1 is not periodic no unit is and there are no trivial cycles; otherwise the trivial cycles are the uC_1, U ∩ C_1 is a subgroup (the stabilizer), and their number is |U|/|U ∩ C_1|, a divisor of |U|; in Z[i] four ({1}), two ({±1}) or one (U). Status: Lean (`unit_periodic_iff`, `unit_cycle_eq`, `stab_iff`, `stab_mul`, `trivial_cycle_count`, `Expr.trivial_cycle_count`); paper for general R. Credit: the orbit-stabilizer theorem; the multiplicity |G|/|G_p| of a symmetry family of cycles is standard (ChaosBook ch. 11, RPA 3.12); not found for Collatz maps. Source: TH Thm 2.1(b); ST Thm 2(2). Papers: P1 Prop 6.2(2); P2 Prop 3.4(2).

T3. Unit cycles of a map commuting with the units: with w = S(1), if w is a unit then S^n(u) = uw^n, every unit lies on a unit cycle of length ord(w), and there are |U|/ord(w) of them; if w is not a unit there is no unit cycle. Status: Lean (`unit_cycles`, `no_unit_cycle`, `Expr.unit_cycles`); paper for general R. Credit: not found (IAP row 7). Source: TH Thm 3.1. Papers: P1 Thm 6.4; P2 Prop 3.4(1).

T4. The number of trivial cycles depends on the associate of α: rotating by u replaces S(1) = w by uw. Status: Lean (`rotate_S_one`); paper. Source: TH Thm 4.4(4); ST 4. Papers: P2 3.3, Table 4.2.

T5. Under the criterion a trivial cycle containing both units and non-units can occur only when S(1) is not a unit; in the census at (1+i)^3 to (1+i)^5 and in the single-cycle maps at (1+i)^7 the cycle through 1 has one odd step, so S(1) is a unit there. Status: paper (from T3) and computed (census cycle tables). Source: TH 3; census entries G31 to G47. Papers: P2 3.3.

T6. Examples of trivial-cycle counts: the 5+2i map two ({1, -1}, {i, -i}); -5-2i with the 5+2i digits four fixed units; 2-5i one cycle through all four units; the below-window map 1+2i at (1+i)^3 one; 7x ± 1 two fixed points; -7x ± 1 the loop 1 ↔ -1; Eisenstein 5+2ω and associates 1, 2, 3 or 6. Status: computed and paper. Source: ST 4 table; ST 10. Papers: P1 6.2, 5.1; P2 3.3, Table 4.2.

T7. Without the criterion both T2 and T3 fail. The 2-i map at (1+i)^2 has the unit cycle 1 → -i → -1 → 1 of length 3 with S(1) a unit and the unit i off it (not periodic). Status: Lean (`E2_cycle`, `E2_I_not_periodic`, `E2_not_symmetric`). Source: TH Remark 3.2. Papers: P1 4.2, 6.4; P2 2.1, 3.3.

T8. Trivial-cycle counts among Collatz maps that do not commute with the units: 1 or 2 at (1+i)^2; at (1+i)^3 tables with 1, 2 and 3 trivial cycles (three for 3i, -3-2i, -2-3i); 1, 2, 3 at (1+i)^4 (three fixed units 1, i, -1 for α = -2-5i, D = U ∪ {1-2i, -2+i, 2-i, -1-2i}); one sampled map with three at (1+i)^6. Three does not divide four. Status: computed to 10^5 (10^6 at (1+i)^3). Source: ST 4; M3 5; LOG variants29_1e5.txt, symtest_k6_sample_B1e5.txt. Papers: P1 4.2; P2 2.2, 2.3, 3.3, 9.

T9. Mixed and lone trivial cycles at (1+i)^3 without the criterion: α = -3i with digits 2-3i, 1, i, -1 (classes of 1, i, -1, -i) has one trivial cycle of ten odd elements, 1 → -2+i → 1-2i → 1+2i → -2-i → 2-i → -1+2i → i → -1 → -i → 1, and no passing -3i table has a unit cycle; α = 3i with digits -2-i, -3-2i, -i, 1 has the fixed unit 1 and a mixed cycle through -1, i, -i and three elements of norm 5; α = 3-2i with 1-2i, -i, 1, i has the lone 2-cycle {i, -i}. Status: computed to 10^7 for the representatives. Source: M3 5, 6. Papers: P1 4.2; P2 2.2.

T10. At norm 29 without the criterion: unit cycles of length 3 (2+5i: 1 → -i → i → 1 with digits 1-2i, -2+i, -2-i, -1-2i; -1 → i → -i → -1 with 1+2i, 2+i, 2-i, -1+2i; also 1 → -1 → i → 1), a lone unit 2-cycle {i, -i} as the only trivial cycle, three fixed units (-2-5i). Status: computed to 10^5. Source: LOG variants29_1e5.txt; INV 2.5.7 (not written up in a research note). Papers: P2 2.3.

T11. On Z there are at most two trivial cycles; with digits D_k and m > 0, S(1) = 1 (fixed points ±1); for -m, S(1) = -1 (loop 1 ↔ -1). A unit loop needs m + d_a = -2^v and -m + d_b = 2^(v'); with smallest digits and m > 0, S(1) > 0, so a loop needs m < 0 or a non-smallest digit. Status: paper. Source: ST 6, 9; IC Lemma 2. Papers: P1 3.2, 6.4.

## 4. Forced fixed points

K1. Fixed-class lemma: for a map with k ≥ 1, v ≥ k, ε a unit and z odd, S(z) = εz with exponent v exactly when δ = (επ^v - α)z lies in D; if z is not a unit, and ε = 1 or the map commutes with the units, the cycle of z (inside {z, iz, -z, -iz}) contains no unit and the map is not a Collatz map. Status: Lean (`Expr.fixed_point`, `Expr.fixed_point_not_rule`); paper for general R. Credit: special cases known on Z: 3x + d fixes d, the trivial cycle scaled by d (Lagarias 1990's non-primitive cycles; Bernstein and Lagarias's scaling; Barina and Maat 2025 for 3n + 3^k); Crandall 1978 proved qx + r with r ≥ 3 has an orbit not reaching 1 (via invariant multiples of r); MW 1985 Example 2 (x/2, (5x-3)/2, two ergodic sets when 3 | m) is an unremarked instance (2^2 - 5 = -1). The general lemma was not found. Source: TH Lemma 5.1; IC Lemma 3; ST Prop 3; IAP 3.5; MW 4.5. Papers: P1 Lemma 6.8, Lemma 3.1 (Z), 8 item 7; P2 Lemma 2.2 (ε = 1), Lemma 4.1.

K2. Corollary: if η = επ^v - α is a unit for some v ≥ k ≥ 1 (ε = 1 unless the map commutes with the units), every digit of a Collatz map is a unit; so it fails for every digit set once there are more odd classes than units. Status: Lean (`Expr.digits_units`). Source: TH Cor 5.2; ST Cor 3a. Papers: P1 Cor 6.9; P2 Cor 2.3, Cor 4.2.

K3. Multipliers reached by K2 with v ≤ k+2: commuting maps at (1+i)^3 all; (1+i)^4: 4 ± i, 5, 4 ± 3i (all but 5 ± 2i); (1+i)^5: 5 ± 4i, 7; (1+i)^6: 8 ± i, 9, 8 ± 7i; (1+i)^7: 9 ± 8i, 15; Z: 2^k+1, 2^(k+1)-1. Non-commuting literal associates (ε = 1): (1+i)^2: -2+i, -1+2i, 1+2i; (1+i)^3: -3, -3+2i, -2+3i; (1+i)^4: -4 ± i, -5, -4-3i, -3-4i; (1+i)^5: -5-4i, -4-5i, -7i; (1+i)^6: ±1-8i, -9i, 7-8i, 8-7i; (1+i)^7: 8-9i, 9-8i, 15. Status: computed. Source: ST 7. Papers: none (P2 2.3 uses the (1+i)^4 cases).

K4. No passing map has a digit in F_α = {(επ^v - α)z : v ≥ k, z odd, N(z) > 1}; F_α accounts for 3728 of 4981 failures among integer maps at moduli 4 to 16, 464 of 566 at integer modulus 8 with 12 candidates, and 147 of 187 among Gaussian maps at (1+i)^3, (1+i)^4; the rest come from longer cycles. Status: computed. Source: ST 7. Papers: none.

K5. Instances in the census: the 5+2i map has -π^5 - (5+2i) = -1+2i = i(2+i) ∈ D, giving the fixed class of 1 with exponent 5 and nothing else; 8 ± i at (1+i)^6 lies at distance 1 from 8 = iπ^6, so every digit gives a fixed class (at least eight per map, matching four maps with 8 cycles); 7 ± 4i, at distance √17, has a single cycle; 11+4i at (1+i)^7 ((8+8i) - (11+4i) = -3+4i) passes only z = 1. Status: paper and computed. Source: IC 5. Papers: P2 5.2.

K6. Structural fixed points: for α = 15 at (1+i)^7 every digit d gives a fixed class (-15(-d) + d = 16d, a unit times π^8 d), 16 in all; for α = 9 at (1+i)^6, S(d) = -id, 8 classes; the heuristic E gives exactly 16 and 8. Status: paper. Source: FAM 2, 3; IC 5. Papers: P2 6.1, 5.2.

K7. Z against Z[i]: the smallest multiplier 2^k+1 is never single-cycle on Z for k ≥ 3 (it has 2^(k-2) fixed points), but in Z[i] the smallest norm 2^k+1 has single-cycle maps at (1+i)^6 (7 ± 4i) and (1+i)^7 (11 ± 4i), because K1 measures the distance from α to the επ^v, not from N(α) to 2^k. Status: paper and computed. Source: IC 1 item 6, 5. Papers: P2 5.2.

K8. Eisenstein: K1 explains the fixed classes: 4 - (5+ω) = ω^2 is a unit (so a Collatz map would need unit digits, and the non-unit digits give fixed points such as 2+ω); 4 - 5 = -1; 4 - (5+2ω) = -1-2ω has norm 3 and lies in D, so only the class of 1 is fixed. Status: paper. Source: IC 7. Papers: P1 5.1.

K9. The window cannot be dropped from Theorem G26: α = 1+2i at (1+i)^3 also gets D = U from K2 (ε = -i, v = 3, η = 1), and that map is a Collatz map (G48). Status: paper. Source: TH 5. Papers: P2 4.1.

K10. Integer cases: at modulus 4, m = 5 and 7 have 2^2 - 5 = -1 and 2^3 - 7 = 1, so a Collatz map has digits ±1 (which commute with -1); for m < 0, 2^v - m is never ±1 and K2 gives nothing. Status: paper. Source: ST Thm 4, 8 item 2. Papers: P1 3.2.

K11. Soundness of the pruning in the modulus-(1+i)^3 search: a digit (π^v - α)z with v ≥ 3, z odd non-unit, forces a nontrivial fixed point; a digit -αz sends z to 0; both lie automatically in the right class. For -3, -3+2i, -2+3i only the unit survives in each class; for the other nine N(π^v - α) ≥ 5 and among the six smallest digits nothing is removed (8 to 44 removed among norm ≤ 100, none for 3, 2-3i, 3-2i, 3+2i). Status: paper (soundness), computed (counts). Source: M3 3, 4. Papers: P2 2.2.

## 5. Maps commuting with the units and the existence threshold

U1. Theorem: for a Dedekind domain R, a nonzero prime P with finite residue field and a finite group U of units, with κ = max over u ≠ 1 of v_P(u - 1), the following are equivalent: (1) some odd α has a map at modulus P^k commuting with U; (2) every odd α has one; (3) k > κ; (4) U acts freely on the odd classes mod P^k. Up to the choice inside each class, a commuting table is one digit for one class in each orbit. Status: paper, not in Lean. Credit: not found (IAP row 4, LEI 3.4, MW 4.4). Source: IC Thm 1, 2.2. Papers: P1 Thm 6.3; P2 3.1 (cited).

U2. Thresholds: Z (κ = 1, from modulus 4); Z[i] (κ = 2 at u = -1 since -2 = iπ^2, from (1+i)^3); Z[ω] (κ = 1: ω - 1 and ω^2 - 1 are odd of norm 3, -ω - 1 and -ω^2 - 1 are units; from modulus 4); other imaginary quadratic fields U = {±1}, κ = e (P^2 if 2 is unramified, P^3 if ramified); with -1 ∈ U and P above 2, κ ≥ e. Infinite unit groups: for every k the kernel of U → (R/P^k)^× contains some u ≠ 1, so no modulus admits a map commuting with all units; commuting with ±1 alone is possible from k > e. Status: paper. Source: IC 2.3. Papers: P1 6.3 table.

U3. Number of smallest-digit commuting tables in Z[i] = product over orbits of the number of least-norm elements in a class of that orbit: 1, 2, 1, 4, 1 at (1+i)^3 to (1+i)^7. Status: paper. Source: IC 2.2. Papers: P2 3.4.

U4. Z[i], k ≤ 2: no map commutes with the units (z and -z share a class since -1 - 1 = iπ^2, so d(z) = 0); at modulus 1 the only unit-closed digit set is {0}, so the map sends odd z to αz. Status: Lean (`prop1`, `pi_sq_dvd_two_mul`, `pi_sq_dvd_neg_two`), formalized as "for k ≤ 2 a digit function depending only on the class with odd values has d(-z) ≠ -d(z) for some odd z". Source: D2 Prop 2; TH 5.4 case k = 2. Papers: P1 4.2; P2 Prop 3.1.

U5. At (1+i)^2 even the weaker form fails: S(-z) cannot always be an associate of S(z), since z and -z share a digit d and (αz + d)/(αz - d) lies within 1/2 of 1 for large |z| while no unit times a power of π other than 1 does. Status: paper. Source: D1 5.1; D2 4.1. Papers: P2 3.1.

U6. Z[i], k ≥ 3: the units lie in distinct classes (their differences have exponent at most 2), act freely, and split the 2^(k-1) odd classes into 2^(k-3) orbits of four; a commuting digit set is the unit multiples of one digit per orbit. At (1+i)^3 each odd class holds exactly one unit (differences of units have norm 2 or 4, nonzero multiples of π^3 norm at least 8). Status: paper; Lean for the k = 3 statement (`units_mod_pi3`, `dvd8_of_pi3`). Source: D2 3.3; TH 5.4. Papers: P1 4.2; P2 3.1, 2.2.

U7. Z at modulus 2: no map commutes with -1 (one odd class, so d(-x) = d(x) = -d(x)); since the window puts ±3 at modulus 2, no map in the window with multiplier ±3 commutes with -1. Status: paper. Credit: not found stated in any source read (SLIT 3.4). Source: D1 5.1; D2 3.6. Papers: P1 Prop 3.3.

U8. For a map commuting with -1 on Z, S(-x) = -S(x), so orbits of negative integers are the negatives of those of positive ones and the problem on Z is the problem on N (Barina notes this for 7x ± 1). Status: paper. Source: SLIT 2.1; D2 3.6. Papers: P1 3.2.

U9. Commuting with the units does not follow from triviality, even with smallest digits: five Collatz maps at (1+i)^2 (G5), 27 at (1+i)^4 (G34), 305 at (1+i)^3 (G14) do not commute; nor does a single cycle pick out a map: free-digit sweeps (digits of norm ≤ 5) give a single nonzero cycle in 68 of 288 maps at (1+i)^2, 108 of 972 at (1+i)^3, 76 of 448 at (1+i)^4. Status: computed to 10^5 (10^6, 10^7 where stated in G entries). Source: ST 1, 11; M3; D1 5.1; COL 3.2. Papers: P2 1, 2, 9.

U10. Known odd maps on Z sit at the moduli U1 allows: Barina's 7x ± 1 and Conway's amusical permutation (OEIS A006368, a(n) = -a(-n); also MW 1985 Example 7, unremarked as odd) at modulus 4 = 2^2 > 2^κ; Collatz's original permutation at modulus 3 (odd, Tremblay arXiv:1907.13302 eq. (1)); Matthews's L_d for odd d (least absolute residues, "balancing"); Kohl's dissertation Example 1.1.3 has a Collatz-type permutation centralizing n ↦ -n. No source proposes commuting with the units as a requirement or classifies such maps by modulus. Status: literature, with our consistency observation. Source: SLIT 3, 5; IAP 3.4; MW 4.4; RPA 3.5. Papers: P1 1.1, 6.3.

U11. The case for the criterion: a conjecture of Collatz type fails by a second cycle or a divergent orbit; Z shows that a map meant to carry such a conjecture on a ring larger than N should commute with the units and count cycles up to units. What it buys is symmetry (orbit of uz is u times the orbit of z, trivial cycles divide |U|, rotated and mirrored maps go together), all of which fail for some non-commuting Collatz maps. Status: argument. Source: D2 1; ST 11. Papers: P2 1.

## 6. Rotated and mirrored maps

R1. Definitions: the rotated map of (α, k, d) by a unit u is (uα, k, d^u) with d^u(z) = d(uz), same digit set; for an automorphism σ of order at most 2 with σ(π) = ηπ (η a unit), the mirrored map is (σ(α), k, σ∘d∘σ). On Z[i], σ is conjugation and η = -i; on Z[ω], η = 1; on Z, σ is the identity. Both are maps. Status: definition; Lean for Z[i] (Variants.lean). Source: TH Def 4.1, 4.5; RPA 1. Papers: P1 Def 6.5; P2 1.2.

R2. One step of the rotated map, any table: S_{uα}(z) = S_α(uz). Status: Lean (`Expr.rotate_S`). Credit: not found, not even on Z as m ↦ -m (RPA 4 item 3). Source: TH Prop 4.2. Papers: P1 Prop 6.6; P2 Prop 3.2(1).

R3. One step of the mirrored map, any table: S_{σα}(z) = η^v σ(S_α(σz)) with v the exponent of w = ασ(z) + d(σz); on Z[i], S_ᾱ(z) = (-i)^v conj(S_α(z̄)); in all cases N(S_{σα}(z)) = N(S_α(σz)). Status: Lean for Z[i] (`Expr.mirror_S`, `Expr.mirror_norm`); paper for general σ (P1). Credit: the factor η^v was not found; Kucinski's Theorem 3 (h(x+yi) = y+xi = i z̄ on Joseph's map) is the nearest precedent, with no factor since Joseph divides by 2 (RPA 3.8, 4 item 4). Source: TH Prop 4.6. Papers: P1 Prop 6.6; P2 Prop 3.2(2).

R4. Transfer lemma: if g commutes with the units and g(z) = ε(z) f(z) with ε(z) a unit for every z, then f trivial implies g trivial (g^n(z) = δ_n f^n(z); g^p(x) = δ'x gives g^(|U|p)(x) = x). Status: Lean (`trivial_transfer`). Credit: standard for maps commuting with a finite group: two such maps agreeing up to a group element induce the same map on R/U, and a relative periodic point is periodic (Field 2007 ch. 2; ChaosBook ch. 11; RPA 3.12). Source: TH Lemma 4.3. Papers: P1 Thm 6.7 proof; P2 Lemma 3.3.

R5. Rotation under the criterion: the rotated map commutes with the units, S_{uα} = uS_α, S^n_{uα} = u^n S^n_α, the norm sequences agree, it is a Collatz map iff the original is, and S_{uα}(1) = uS_α(1). Status: Lean (`Expr.rotate_symmetric`, `rotate_S_mul`, `rotate_iter`, `rotate_norm`, `rotate_rule`, `rotate_S_one`). Source: TH Thm 4.4. Papers: P1 Thm 6.7(1); P2 Thm 3(1).

R6. Reflection under the criterion: the mirrored map commutes with the units, S^n_ᾱ(z) = (-i)^(V_n(z̄)) conj(S^n_α(z̄)) with V_n the total exponent, the norm sequence of z under the mirror is that of z̄ under the original, and the mirror is a Collatz map iff the original is. Status: Lean (`Expr.mirror_symmetric`, `mirror_iter`, `mirror_norm_iter`, `mirror_rule`); paper for general σ with η^(V_n). Source: TH Thm 4.7. Papers: P1 Thm 6.7(2); P2 Thm 3(2).

R7. If η = 1 (σ fixes π, as conjugation on Z[ω]), then S_{σα} = σ∘S_α∘σ for every table, commuting or not, and the mirror is a Collatz map iff the original is; so in Z[ω] the mirror of every map is conjugate to it, and the verdicts for 5+2ω and 5+3ω agree. Status: paper (P1 Thm 6.7(3)); the research files have the observation that σ S_α σ^(-1) divides by σ(π) (RPA 1) but not the theorem. Papers: P1 Thm 6.7(3), 5.1.

R8. Decomposition: rotation is conjugation by u (multiplier kept, digit u^(-1)d(uz)) followed by multiplying the output by u; the mirror is conjugation by σ followed by a change of generator of the prime, whose price is η^v. Consequently a map that does not commute with the units depends on the choice of the generator π of its prime. Status: paper (remark). Source: RPA 1, 4; ST 2; TH 4. Papers: P1 6.5.

R9. Without the criterion the relation stops after one step: the second step of the mirrored orbit is applied to η^v σ(S_α(σz)) and that of the rotated orbit to u S_{uα}(z), inputs an arbitrary table treats as unrelated. Examples of opposite verdicts: 2 - i and 2 + i at (1+i)^2; 3i and 3 at (1+i)^3; at norm 29 the rotations of 5+2i pass 32 times and those of 5-2i 16 times over the 128 smallest-digit tie-breaks, and -5-2i, 5-2i, -5+2i pass only with the commuting table. Status: paper (mechanism) and computed. Source: ST 2, 5.1; TH 4; M3 5; LOG variants29_1e5.txt; INV 2.5.8. Papers: P1 4.2; P2 2.1, 2.3, 3.2.

R10. A map commuting with the units comes in a family of up to 2|U| maps, the orbit of (α, d) under the rotations and the mirror, all Collatz maps together with the same norm sequences up to z ↦ σz; eight in Z[i]; on Z the family of mx ± 1 is {m, -m} with S_{-m} = -S_m. If D is closed under conjugation, (α, D) and (ᾱ, D) are mirror images; in general (α, D) and (ᾱ, D̄). Status: paper. Source: TH 4; RPA 4 item 5, 5 item 5; D2 3.2. Papers: P1 6.5; P2 3.2.

R11. The family of the 5+2i map (rotations with D = U ∪ (2+i)U, rotations of the mirror with D̄ = U ∪ (2-i)U): S(1) and trivial cycles are 5+2i: -1, two; -2+5i: -i, one (1 → -i → -1 → i); -5-2i: 1, four fixed units; 2-5i: i, one; 5-2i: i (= (-i)^5 conj(-1)), one; 2+5i: -1, two; -5+2i: -i, one; -2-5i: 1, four. All trivial cycles are unit cycles. Status: paper (direct multiplication with R5, R6, T3) and computed to 10^5. Source: TH 4; ST 4; LOG variants29_1e5.txt. Papers: P2 Table 4.2.

R12. Numerical check of R5 and R6 on the 5+2i family: the first 60 norms of every odd z with |a|, |b| ≤ 60 under each of the eight maps against those of z or z̄ under the 5+2i map, 58,560 comparisons, no mismatch (`mirror29.py`). Status: computed. Source: INV 3.3.8 only (run for the inventory; no log or research note). Papers: P2 3.2.

R13. Prior art for conjugating by a unit (multiplier kept, digit moved): on Z, 3x+1 on the negatives is 3x-1 (folklore; OEIS A037084; Gonçalves, Greenfeld and Madrid arXiv:2111.06170 Remark 1.7); Q(x) = kx carries 3x+1 to 3x+k on Z_2 (Bernstein and Lagarias 1996; Lagarias via Matthews Example 5.1); Fraboni's classification of linear conjugacies of 2-branch maps on Z_2 (slides only); on Z[i] the REU associates proposition. Integer conjugacies of one map: Monks and Yazinski's autoconjugacy, Kohl's `Mirrored` (n ↦ -n-1). None rotates the multiplier. Status: literature. Source: RPA 3. Papers: P1 6.5, 8 item 8.

R14. The REU associates proposition (z reaches u_1 under digit u_1 iff u_2 z reaches u_1 u_2 under digit u_1 u_2) is the modulus-(1+i) content of the absence of commuting maps there: the units permute the digit-u maps instead of fixing one. Status: remark. Source: LIT 2.3, 3.2; IAP 3.4. Papers: none.

## 7. Smallest digits and ties

D1. If every odd class mod π^k has a unique element of least norm, the table of these elements commutes with the units. Status: paper. Source: ST Lemma 1. Papers: P1 6.7; P2 Lemma 3.5.

D2. Least-norm elements of a class lie in the closed Voronoi cell of π^k R at 0, and ties occur only on its boundary. In Z[i]: odd k = 2j+1, cell |x| + |y| ≤ 2^j, boundary points even for j ≥ 1, so no ties (except at modulus 1+i); even k = 2j, cell |x|, |y| ≤ 2^(j-1), 2^j tied classes with two least elements each, 2^(2^j) smallest-digit tables, 2^(2^(j-2)) of them commuting (j ≥ 2). Status: paper. Credit: least-norm remainders and Voronoi ties are standard (Euclidean algorithm); the tie counts were not found; on Z the precedent is Matthews's L_d (least absolute residues, odd for odd d, tie at ±d/2 broken toward +d/2 for even d, so not odd). Source: ST Prop 1, 3; IAP 3.6. Papers: P1 4.2, 6.7; P2 Prop 3.6.

D3. Ties table: (1+i)^2: 2 odd classes, 2 tied (norm 1), 4 tables, 0 commuting, 1 orbit; (1+i)^3: 4, 0, 1, 1, 1; (1+i)^4: 8, 4 (norm 5), 16, 2, 6; (1+i)^5: 16, 0, 1, 1, 1; (1+i)^6: 32, 8 (four of norm 17, four of 25), 256, 4, 70; (1+i)^7: 64, 0, 1, 1, 1; orbits by Burnside, (16+4+2+2)/4 and (256+16+4+4)/4. At (1+i)^2 (j = 1) -1 swaps the two elements of each tied pair, so no tie-break commutes. Z, moduli 4 to 64: unique. Status: computed (`symtest.py ties`) and paper. Source: ST 3. Papers: P2 3.4 table.

D4. So at odd moduli the smallest digits already commute with the units, the census there contains every Collatz map with smallest digits, and only even moduli carry smallest-digit maps that do not commute. Status: paper. Source: ST 3, 11. Papers: P1 4.2; P2 3.4.

D5. Commuting smallest-digit tables by modulus: (1+i)^3: U; (1+i)^4: U with the unit multiples of 2+i, or of 1+2i (conjugates); (1+i)^5: unit multiples of 1, 3, 1+2i, 1-2i; (1+i)^6: unit multiples of 1, 3, 1 ± 2i, 2 ± 3i with those of 1+4i or 1-4i and of 3+4i or 3-4i, named A (1-4i, 3-4i), B (1-4i, 3+4i), C (1+4i, 3-4i), D (1+4i, 3+4i), with A, D and B, C mirror pairs and none closed under conjugation; (1+i)^7: unit multiples of 1, 3, 5, 7, 1 ± 2i, 1 ± 4i, 1 ± 6i, 2 ± 3i, 2 ± 5i, 3 ± 4i, unique and closed under conjugation. Status: computed exactly, reproduced by an independent division test. Source: D2 4.4; COL 5 item 1; FAM 2. Papers: P2 3.4.

D6. Z: for k ≥ 2 each odd class mod 2^k has a unique element of least absolute value, forming D_k = {odd d : |d| < 2^(k-1)}, closed under negation, so the D_k map commutes with -1; it is also the unique smallest-digit set without the criterion; with 2^k < m < 2^(k+1), S(1) = 1. The only tie on Z is ±1 at modulus 2, which is why 3x+1 and 3x-1 both use smallest digits. Status: paper. Source: IC Lemma 2; ST Prop 1(3). Papers: P1 Lemma 3.4, 3.2.

D7. Commuting with the units and triviality do not force smallest digits: passing digit sets share no visible feature beyond avoiding F_α (digits up to 47 at integer modulus 8, outside the Voronoi cell; at (1+i)^4 the commuting map -5-2i with D = U ∪ (5-2i)U, digits of norm 29). Status: computed. Source: ST 7. Papers: P2 4.1.

D8. Z[ω]: at modulus 4 each of the 12 odd classes has a unique least element (the units and the associates of 1-ω, norm 3), so one table U ∪ (1-ω)U, commuting; at modulus 8 one of the eight orbits is tied (2-3ω and 2+5ω, norm 19), so two conjugate commuting tables and 64 smallest-digit tables in all. Status: paper and computed. Source: IC 7; ST 10. Papers: P1 5.1, 6.7.

## 8. Results on N and Z

### 8.1 3x+1, 3x-1 and multiplier ±3

Z1. 3x+1 on Z has, besides 0 and 1 → 4 → 2, the cycles -1 → -2, a cycle of 5 steps through -5 (-5 → -14 → -7 → -20 → -10) and one of 18 steps through -17; Böhm and Sontacchi conjecture these are all; 3x-1 on N is 3x+1 on the negatives (OEIS A037084, Kamenetsky 2017; Cloitre's three cycles with minima 1, 5, 17). Status: literature (Lagarias 1985; Encyclopaedia of Mathematics). Source: SLIT 3.4. Papers: P1 1.1; P2 1.

Z2. 3x+1 does not commute with -1: -(3x+1) = 3(-x) - 1. Status: paper. Source: D2 1. Papers: P1 1.1; P2 1.

Z3. No map with multiplier 3 at modulus 2 is a Collatz map on Z: the single digit δ is fixed by K1 since (2^2 - 3)δ = δ, so δ = ±1, and 3x ± 1 have the negative cycles. Status: paper, assembled in P1 from K1 (fixed-class lemma) and Z1; not stated as a proposition in a research file. Source: ST 6; IAP 3.5. Papers: P1 Prop 3.2.

Z4. -3x ± 1 have a cycle of 12 odd steps through ±13; for m = -3, 2^v + 3 is never ±1, so K1 with ε = 1 gives nothing; other digits for -3 were not tried. Status: computed to 10^6. Source: ST 6. Papers: P1 3.2, 7 item 3.

Z5. 3x+1 accelerated: worst case 3/2, average 3/4; measured on x^2 (to compare with norms) 9/16, -0.83 bits per odd step. Status: paper. Source: D1 1, 4; D2 6.4. Papers: P2 4.3.

Z6. Smallest-digit integer maps of both signs at moduli 2 to 32, odd 0 < |x| ≤ 10^6, literal cycles: modulus 2 none of ±3x ± 1 passes (3x+1 has -5, -17; 3x-1 has 5, 17; -3x ± 1 the 12-step cycle); modulus 4: m = ±7; 8: none; 16: ±21, ±25; 32: ±41, ±43, ±49, ±53; exactly the single-cycle maps of the census (Z14). For m > 0 the trivial cycles are the fixed points ±1, for m < 0 the loop 1 ↔ -1. Status: computed to 10^6. Source: ST 6; LOG symtest_Z_k1_B1e6.txt, symtest_Z_k2-5_B1e6.txt. Papers: P1 3.2 (partly).

Z7. On Z, x ↦ -x conjugates (m, D) to (m, -D); -m is a different map; for D closed under sign, T_{-m} = -T_m on odd steps. Status: paper. Source: ST 2. Papers: P1 6.5.

### 8.2 Modulus 4: 7x ± 1 and 5x ± 1

Z8. 7x ± 1 (digit +1 for x ≡ 1, -1 for x ≡ 3 mod 4) and 5x ± 1 (digit -1 for x ≡ 1, +1 for x ≡ 3) are the two maps in the window at modulus 4 that commute with -1. Status: paper. Source: D2 3.6. Papers: P1 3.2.

Z9. Barina's 7x ± 1 (arXiv:1807.00908; CMST 28(4) (2022) 143-147): conjecture that every positive orbit reaches 1, verified to 10^15, longest orbit below 10^10 of 5717 steps from 6,346,893,259, negative orbits negated positive ones, only the cycles through 1, -1, 0, heuristic (1/2)log(7/4) + (1/2)log(1/2) < 0; OEIS A317640, A317753, A318489, A318972 and the wiki page "7x±1 problem". He does not derive it from a symmetry requirement, frame it by modulus, consider 5x ± 1, or say 3x+1 cannot commute with -1; Villatoro's 2020 survey lists it; no research paper citing it found. Status: literature (read in full). Source: SLIT 2. Papers: P1 1.1, 3.2.

Z10. Our check of 7x ± 1: every odd 3 ≤ x ≤ 10^7 falls below x, so every odd |x| ≤ 10^7 reaches ±1. Status: computed to 10^7 (`zsym.py`); superseded as a result by Barina's 10^15 and kept as a test of the code. Source: D2 3.6; SLIT 6. Papers: P1 3.2.

Z11. 5x ± 1 has two cycles up to sign: the fixed point 1 and 7 → 36 → 18 → 9 → 44 → 22 → 11 → 56 → 28 → 14 → 7 (accelerated 7 → 9 → 11 → 7). Status: computed. Credit: not found; SLIT could not run OEIS's own search and recommends a direct search for 1, 1, 3, 9, 11, 7, 1, 19, 21 and 4, 1, 16, 2, 24, 3, 36, 4, 44, 5, 56 before claiming it unrecorded. P1 3.2 states that such OEIS searches were run and found nothing, and that up to sign the stretch 36, 18, 9, 44, 22, 11, 56, 28, 14, 7 occurs in del Arte's orbit of 5 under -5x+1 (OEIS A305057) without closing; neither the search nor A305057 is recorded in any research file. Source: SLIT 4, 6. Papers: P1 3.2.

Z12. Why 7x ± 1 has one cycle and 5x ± 1 two: both have δ = 1, so 1 is the only positive fixed point (Z13), and the second cycle of 5x ± 1 is the 3-cycle through 7. Status: paper and computed. Source: IC 5. Papers: P1 3.2.

### 8.3 Smallest-digit maps at moduli 2^k: fixed points and census

Z13. Fixed points of the D_k maps: for k ≥ 2, 2^k < m < 2^(k+1), δ = min(m - 2^k, 2^(k+1) - m), odd x > 0 is fixed iff x < 2^(k-1)/δ; no S(x) = -x; ⌊(2^(k-1)/δ + 1)/2⌋ fixed points up to sign, at least two iff m < (7/6)2^k or m > (11/6)2^k; m = 2^k+1 and 2^(k+1)-1 have 2^(k-2). So from modulus 8 on the outer sixths of every window hold no Collatz map with smallest digits. Status: paper; matches the census for all 1016 maps run. Credit: not found (IAP row 11); Barina's map is the case k = 2, m = 7. Source: IC Prop 4, 3.2. Papers: P1 Prop 3.5.

Z14. Integer census of the D_k maps at moduli 4 to 1024, 1022 maps: Collatz maps (single cycle up to sign at 10^6, each confirmed by a drop-below-start check for odd 3 ≤ x ≤ 10^7) 1, 0, 2, 4, 5, 15, 30, 67, 127 at moduli 4, 8, ..., 1024, 251 in all; every other map has two cycles exhibited and checked independently in exact arithmetic (765 by `zverify.py`) or given by Z13. Status: computer-assisted (Proposition 6 of IC). Source: IC 1, 4, 8; LOG zsummary_sym.txt, zcensus_sym_drop_1e7.txt, zverify.txt. Papers: P1 3.2, Table 1.

Z15. Census columns by modulus 4 to 1024: first single-cycle m 7, none, 21, 41, 77, 153, 307, 611, 1223; excluded by Z13 0, 2, 2, 6, 10, 22, 42, 86, 170; cycles of the smallest m (5: 2, 9: 2, 17: 4, 33: 8, then 16, 32, 64, 128, 256) and of the largest (7: 1, 15: 3, 31: 7, 63: 12, 127: 16, then 32, 64, 128, 256); most cycles 5: 2, 15: 3, 31: 7, 63: 12, 97: 50, 181: 294, then 257/511, 513/1023, 1025/2047. Status: computed to 10^6. Source: IC 4.2. Papers: P1 Table 1 (first three columns).

Z16. Single-cycle lists: modulus 4: 7; 16: 21, 25; 32: 41, 43, 49, 53; 64: 77, 85, 99, 107, 111; 128: 153, 155, 157, 165, 167, 173, 193, 195, 197, 201, 207, 211, 213, 227, 229. Modulus 8 cycles: m = 9 fixed 1, 3; 11: 1 and a 2-cycle through 5; 13: 1 and 10 odd steps through 151; 15: 1, 3 and 43 odd steps through 89. Status: computed. Source: IC 4.2; LOG zsummary_sym.txt. Papers: none.

Z17. The 251 single-cycle maps have m/2^k in [1.193, 1.832] inside (7/6, 11/6), drift in [-0.745, -0.127] bits (natural log [-0.5164, -0.0877]) against the allowed (log2(7/12), log2(11/12)) = (-0.778, -0.126); their share settles near a quarter from modulus 128 on (23%, 23%, 26%, 25%). Status: computed and paper. Source: IC 4.3, 5. Papers: P1 3.2.

Z18. By tenths of the window, single-cycle maps / maps: 0/102, 7/102, 45/103, 46/102, 0/102, 51/102, 40/102, 47/103, 15/102, 0/96; the tenth around √2 is empty because of 2-cycles with V = 2k+1 (at modulus 1024 every m from 1391 to 1469 has one, 53 for m = 1449 where 1449^2/2^21 = 1.0012); thinner gaps near 2^(1/3), 2^(2/3), 2^(1/4), 2^(3/4) (at 1024, m = 1255 to 1309 and 1611 to 1653). Status: computed to 10^6. Source: IC 4.3. Papers: P1 3.2.

Z19. No divergence on Z at any modulus to 1024 (escape threshold 2^2000, peaks to 2^1322 for m = 255); eleven maps hit the step cap (127; 255; 509, 511; 1017 to 1023; 2031, 2033, 2035), all in the range of Z13, drift -0.0083 to -0.0010. At modulus 1024 with m ≥ 2033 the run at 10^5 found exactly the 17 and 20 fixed points of Z13 for 2033 and 2035, with 1 and 26 starts at the cap. Integers have no analogue of the invariant lines, and every integer map in the window has negative drift. Status: computed (10^6; 10^5 at the top of modulus 1024). Source: IC 4.1, 4.2; LOG zcensus_sym_k10top_B100000.txt. Papers: P1 3.2, 4.2.

Z20. Integer resonances, heuristic E with |U| = 2 (E = 2^((k-1)n) C(V-kn+n-1, n-1)/(2n|2^V - m^n|)): m = 181 at 128 (n = 2, V = 15, 181^2 = 0.999786·2^15) 293 or 294 cycles against E = 292.6; 1449 at 1024 (2, 21) 53 vs 53.5; 1447: 41 vs 39.2; 645 at 512 (3, 28) 85 vs 84.5; 97 at 64 (5, 33) 49 vs 45.3; 1505 at 1024 (9, 95) 92 vs 89.9; 363 at 256 (2, 17) 12 vs 11.8; sweeps across √2 at 512 and 1024 within about 15%. Status: computed counts, heuristic E. Credit: E not found in print (IAP row 9; Lagarias 1990 unread). Source: IC 4.4; LOG zresonance.txt. Papers: P1 6.7.

Z21. Integer cycles at convergents: the 3x+1 cycle through -17 has n = 7, V = 11, an intermediate fraction of log2 3 (a record of the one-sided problem 2^V < 3^n forced for x < 0), the cycles at -1, 1, -5 also at records; 5x+1: 13 and 17 at n = 3 (7/3); 181x+1: 27 and 35 at n = 2 (15/2; 181^2 = 2^15 - 7, attributed to Crandall); no other cycles for m ≤ 31 (|min| ≤ 2·10^5). With digits ±1 at modulus 4, m odd 3 to 31: only m = 5 (±7, n = 3), 9 (±13, n = 6), 11 (±1, n = 2, V = 7), 13 (±1, n = 3, V = 11, and ±25, ±49, n = 10) besides ±1; none for m ≥ 19. Status: computed to 2·10^5. Credit: arXiv:2005.00346 lists 181x+1 and 5x+1 cycles. Source: CONV 5; LOG convergents_int_plus.txt, convergents_int_sym.txt. Papers: none.

Z22. Scores of the commuting integer Collatz maps: 7x ± 1 0.1071 (drift -0.193), 25x ± 1 0.0225 (-0.356), 21x ± 1 0.0149 (-0.608), 53x ± 1 0.0124 (-0.272), 49x ± 1 0.0108 (-0.385), 43x ± 1 0.0080 (-0.574), 41x ± 1 0.0069 (-0.642); 3x+1 on N 0.167. Status: computed. Source: SC 3. Papers: P1 2.3, 3.2.

Z23. Z against Z[i]: first commuting modulus 4 against (1+i)^3; the first single-cycle map comes at the first commuting modulus in Z and at the second in Z[i]; in both rings the first single-cycle map is the largest multiplier at its modulus (7 at 4; 5+2i at (1+i)^4), and the next modulus has none (8; (1+i)^5). Status: paper and computed. Source: IC 1 item 6, 5; D2 3.6. Papers: P2 4.2 (5+2i largest at its modulus not stated), 5.1.

### 8.4 Non-smallest digits and maps that do not commute with -1

Z24. Non-smallest maps commuting with -1 that pass to 10^6: modulus 4, digits |d| < 32 (32 maps): only 7x ± 1; modulus 8, 12 candidates (576 maps): 10 pass, none smallest, e.g. m = 11 with ±1, ±13; m = 11 with ±17, ±3; m = 13 with ±1, ±5; m = 11 with ±47, ±37; modulus 16, 5 candidates (5000 maps): 125 pass, 2 smallest. Status: computed to 10^6. Source: ST 7; LOG symtest_Z_symnonmin_*. Papers: none.

Z25. Maps not commuting with -1 at modulus 4, 80 random tables with |d| ≤ 15 and m = ±5, ±7: six pass, all with m < 0 (e.g. m = -5 with digits 13, 3; m = -7 with -3, 11), as K10 requires. Status: computed to 10^6. Source: ST 8 item 2; LOG symtest_Z_nonmin_k2.txt. Papers: P1 3.2.

Z26. Unit-loop maps (1 ↔ -1): at modulus 2 only -3x ± 1, which fail; modulus 4: 82 maps with the loop, 5 pass (-7x ± 1 and four with m = -5: digits (1, 11), (-11, -1), (-3, 59), (-59, 3)); modulus 8: 2500 run, 106 pass (m = -9: 102, -11: 3, -13: 1), two commute with -1; modulus 16: sample of 3000 of 75816, 201 pass, including m = 21 with digits 17, 3, 37, 23, -7, -37, 13, -1 and m = 25; 7x ± 11 has the loop but fails. Status: computed to 10^6. Source: ST 9; LOG symtest_Z_loop_*. Papers: P1 3.2.

### 8.5 Least positive digits (maps preserving N)

Z27. With the least positive residues 1, 3, ..., 2^k - 1 (as in 3x+1) and 2^k < m < 2^(k+1): 1 and -1 are fixed for every m, so no such map is a Collatz map on Z; odd x > 0 is fixed iff x < 2^k/(2^(k+1) - m) (3 fixed iff m > (5/3)2^k); odd x < 0 iff |x| < 2^k/(m - 2^k) (so -3 fixed iff m < (4/3)2^k); no S(x) = -x. These maps preserve N. Status: paper. Source: IC Prop 4(3), 6 (the S(x) = -x and the -3 statements are derived in P1 8 item 3 from the fixed-point lemma). Papers: P1 8 item 3.

Z28. Least-positive-digit maps with m < (5/3)2^k at moduli 4 to 1024, all odd |x| ≤ 10^5, cap 10^4: single positive cycle counts 0, 1 (m = 9), 2 (17, 23), 3 (33, 37, 51), 5, 15, 33, 55, 112; single negative cycle 0, 1 (m = 11), 0, 2, 1, 6, 9, 20, 36; the smallest multiplier 2^k+1 has a single positive cycle at every modulus 8 to 1024, and single-cycle maps crowd to the bottom of the window, the reverse of the commuting maps. Which are Collatz maps on Z in (4/3)2^k < m < (5/3)2^k was not checked. Status: computed to 10^5. Source: IC 6; LOG zcensus_pos_k2-10_B100000_mlt5over3.txt. Papers: P1 8 item 3.

### 8.6 Other integer maps from the literature

Z29. Odd maps on Z in the literature: Collatz's original permutation g (2n/3, (4n-1)/3, (4n+1)/3 by n mod 3), odd (Tremblay arXiv:1907.13302 eq. (1)); Conway's amusical permutation A006368 (3x/2 on evens, (3x ± 1)/4 on odds; below the window), also MW 1985 Example 7; Matthews's L_d with least absolute residues (Carnielli, Appl. Math. E-Notes 15 (2015); Tremblay eq. (43)), odd for odd d, orbits reported to reach 1 or -1 by sign. Status: literature. Source: SLIT 3.1 to 3.3; MW 2.5. Papers: P1 1.1.

Z30. Lu Pei's L_3: every 1 ≤ x < 10^5 reaches 1; L_5, L_7 have further cycles. Status: computed to 10^5 (scratchpad `lupei.py`, not in the repository). Source: SLIT 3.3. Papers: P1 8 item 9.

Z31. MW 1985 takes positive multipliers; Leigh 1986 allows any nonzero m_i (negative branches in his Example 1), with |m_i/d| in his criterion, so the window does not see the sign of the multiplier and covers -m. Status: literature. Source: MW 5.1; LEI 2.1, 4. Papers: P1 2.2, 3.3.

## 9. Results on Z[i] by modulus

### 9.1 Moduli 1 and 1+i

G1. No map at modulus 1 or 1+i lies in the window: (1, 2) holds no integer and N(α) = 3 is not a sum of two squares; the shape of 3x+1 (one odd class, one digit) has no counterpart inside the window. Status: Lean (`prop3_int`, `prop3`, `sq_cases`; the step from the window to N(α) = 3 is not formalized). Source: D1 Prop 3; D2 Prop 4. Papers: P1 4.2; P2 2.

G2. At modulus 1+i the four units are tied in the single odd class. Status: paper. Source: ST Prop 1(1). Papers: none.

G3. The REU map C_{2+i,u} (Alvarado and the 2017 CSU Channel Islands REU: z/(1+i) on even z, (2+i)z + u on odd z, u a fixed unit) lies at modulus 1+i with N(α) = 5 above (2, 4); its drift log2 5 - 2 = +0.32 bits per odd step fits the report that about 5.6% of 23,730 starts with |a|, |b| ≤ 100 reached a unit within 10,000 steps. Other REU content: associates proposition (R14), closed forms for P_n with C(P_n) = (1+i)^(2n), coalescence mod 5, an Eisenstein analogue dividing by 1-ω without computations. Status: literature (slides only; the paper not found) and paper (drift). Source: LIT 2.3. Papers: P1 4.2; P2 9.

### 9.2 Modulus (1+i)^2: Collatz maps that cannot commute

G4. The multipliers are the eight associates of 2 ± i (norm 5, the smallest odd prime norm). Classes are classes mod 2, odd classes those of 1 (a odd, b even) and i; tied least elements ±1, ±i; four smallest-digit tables forming one conjugacy orbit, none commuting; so eight maps up to conjugacy, one per multiplier, D = {1, i}. Status: paper and computed. Source: ST Prop 1(2), 3, 5.1. Papers: P1 4.2; P2 2.1.

G5. Of the eight, five are Collatz maps at 10^5: 2-i (unit cycle 1 → -i → -1 → 1), -1-2i (1 → -1 → -i → 1), -2-i (1 → i → -1 → -i → 1), 1+2i (fixed -1, -i), -2+i (fixed 1, -i); three fail: 2+i (trivial cycle through all units plus a cycle through 5-2i, 1-6i, -5-4i, -6+3i, 7i), 1-2i (trivial {1, -1}, {i, -i}; cycles {1+2i, -3i}, {-2-3i, 1+4i}), -1+2i (four fixed units and five cycles through -1-2i, 2-i, 3-2i, 1-4i, 4+i). The four tables of each multiplier give identical counts. Status: computed to 10^5. Source: ST 5.1; LOG symtest_k2_B1e5.txt. Papers: P1 4.2; P2 2.1.

G6. The five passing maps under `gstopall` over all quadrants for 1 < N(z) ≤ 10^7, 15,708,048 starts each: no failure, no cycle; longest stopping times 203, 210, 216, 260, 236 steps of T; mean 7.76 to 7.77; largest excursions 10.06 to 11.38 bits; independent Python check to 10^4. So every orbit with N(z) ≤ 10^7 reaches a unit (by V2). Status: computed to 10^7. Source: ST 5.1; LOG symtest_k2_gstopall_1e7.txt (per-map figures only in the log). Papers: P1 4.2; P2 2.1.

G7. The 2-i map E_2: T(z) = z/(1+i) for a+b even, (2-i)z + i for a odd b even, (2-i)z + 1 for a even b odd; its table is a correct digit table, it divides on even inputs, T(z) ≠ 0 for z ≠ 0, S(1) = -i, S(-i) = -1, S(-1) = 1, S(i) = -i. Status: Lean (`digit2_class`, `digit2_correct`, `E2_T_div`, `E2_cycle`, `E2_I_not_periodic`, `E2_T_ne_zero`, `E2_not_symmetric`). Source: TH Prop 6.2. Papers: P1 4.2; P2 Prop 2.1.

G8. E_2 soundness: if every odd z with 1 < N(z) ≤ B drops below its norm, every nonzero z with N(z) ≤ B reaches {1, -i, -1} (T^4(i) = -i via i → 2+2i → 2 → 1-i → -i); the run gives B = 10^7. Status: Lean (`E2_sound`) plus computer-assisted. Source: TH Cor 6.3. Papers: P1 4.2; P2 Thm 1.

G9. E_2 has drift log2(5/8) = -0.68 bits (ln(5/8) = -0.47) per odd step, the strongest contraction inside the window, largest excursion 11.4 bits, score 1/4 - 1/5 = 0.05. Status: computed. Source: ST 5.1; SC 3. Papers: P1 4.2; P2 2.1, 5.4.

G10. Where E_2 orbits enter the trivial cycle: of 141,324 odd starts with N(z) ≤ 90,000 other than 1, -i, -1, i, 76,511 enter at 1, 40,130 at -i, 24,683 at -1, and 17,979 pass through i. Status: computed (`entry2mi.py`). Source: INV 2.4.12 only (run for the inventory; no log or research note). Papers: P2 2.1.

G11. The mirror of E_2 has multiplier 2+i and digit set {1, -i} = -i{i, 1}, so it is conjugate to the failing 2+i map: conjugation is not a symmetry of non-commuting maps. These five maps are the Gaussian counterpart of 3x+1 that the criterion rules out, at norm 5 against 29. Status: paper and computed. Source: ST 1 item 1, 5.1. Papers: P1 4.2; P2 2.1.

G12. Free-digit sweeps at (1+i)^2 (digits of norm ≤ 5): all 288 maps converge at 10^5, 68 with a single nonzero cycle, none commuting (these counts predate the triviality criterion and count cycles differently). Status: computed to 10^5 (`sweep_gen.py`). Source: D1 5.1; D2 3.2; COL 3.2. Papers: P1 4.2; P2 9.

### 9.3 Modulus (1+i)^3 without the criterion

G13. A table is a list (d_1, d_i, d_{-1}, d_{-i}), d_u in the class of -αu; the unit table D = U commutes; the twelve multipliers are the associates of 3 (norm 9) and 3 ± 2i (norm 13); without the criterion associates and conjugates are different maps and only z ↦ uz remains as conjugacy. Status: paper. Source: M3 2. Papers: P2 2.2.

G14. Search with the six smallest digits per class (norms 1, 5, 5, 9, 13, 13; 1296 tables per multiplier, 15552 in all): 5628 survive pruning, 1212 pass at 10^4 (all quadrants, 15,700 starts; confirmed through `gcz` with exact re-iteration, plus 410 random failing tables, no disagreement), 1204 pass `gstopall` to 10^6 (1,570,740 starts each), 305 maps up to conjugacy, over 8 of the 12 multipliers; none of the five commuting tables per multiplier passes. By multiplier (pass at 10^4 / 10^6 / maps; tables with 1, 2, 3 trivial cycles; tables with a unit cycle; max stopping time; max excursion): 3: 0; 3i: 240/240/61; 172, 52, 16; 136; 184; 7.1; -3: 0; -3i: 48/48/13; 46, 2, 0; 0; 184; 7.3; 3+2i: 72/72/18; 72, 0, 0; 24; 1810; 36.3; -2+3i: 0; -3-2i: 124/124/31; 32, 60, 32; 120; 1838; 37.3; 2-3i: 84/84/21; 80, 4, 0; 60; 1627; 44.9; 3-2i: 176/168/42; 148, 20, 0; 92; 1862; 38.2; 2+3i: 272/272/69; 268, 4, 0; 144; 1867; 43.9; -3+2i: 0; -2-3i: 196/196/50; 74, 74, 48; 172; 2191; 42.1. Status: computed to 10^6. Source: M3 1, 3, 5; LOG m3search_n6_B1e4.txt, m3search_n6_gstopall_1e6.txt. Papers: P1 4.2; P2 2.2 table.

G15. How many digits must change: tables passing at 10^6 with a single non-unit digit number 4 (-3i), 16 (3i), 8 (-2-3i), 4 (2-3i, with a digit of norm above 5), 16 (2+3i), 8 (3-2i); none for -3-2i and 3+2i, which need at least two. A small change to the unit table removes the nontrivial cycles of G26. Status: computed to 10^6. Source: M3 6. Papers: P1 4.2; P2 2.2.

G16. The map α = 3i with digits 2-i, -1, -i, 1 (differs from the unit table (-i, 1, i, -1) in the class of 1) has the single trivial cycle 1 → -i → -1 → i → 1 (3i + (2-i) = π^3(-i), 3i(-i) + 1 = π^4(-1), 3i(-1) - i = π^4 i, 3i·i - 1 = π^4), and every odd z with N(z) ≤ 10^7 drops below its norm, longest stopping time 155. Status: paper (the cycle, by hand) and computed to 10^7. Source: M3 1, 6. Papers: P1 4.2; P2 2.2.

G17. Representatives (least total digit norm) per multiplier, each through `gstopall` to 10^7 (15,708,048 starts, no failure): -3i (2-3i, 1, i, -1; one mixed cycle; 142); 3i (2-i, -1, -i, 1; unit cycle; 155); -3-2i (-1+2i, 2-i, -1, -i; fixed units 1 and i; 1556); -2-3i (2-i, -1, -i, 1; -1 → -i → 1 → i; 1857); 2-3i (2-i, 1+2i, -i, 1; -1 → 1 → i → -1; 1476); 2+3i (2+i, 1, i, -1; -1 → i → 1 → -1; 1932); 3-2i (1-2i, -i, 1, i; -i ↔ i; 1514); 3+2i (1-2i, 2+i, -1-2i, i; -1 ↔ 1; 1575). Scores 1/72 (norm 9) and 5/104 (norm 13). Status: computed to 10^7. Source: M3 6; LOG m3search_reps_gstopall_1e7.txt. Papers: P2 2.2 (partly).

G18. The eight tables passing at 10^4 and failing at 10^6 all have α = 3-2i (two conjugacy orbits, representatives (1+2i, -i, 1, i) and (-3-2i, 3i, -3, i)); each has a nontrivial cycle of 187 odd steps, least elements 31+236i (norm 56657) and -108-213i (norm 57033), largest norms 28 and 30 bits, verified in exact arithmetic; starts such as -60-177i, 59+224i fall in without dropping. Status: computed. Source: M3 5. Papers: P2 2.2.

G19. Theorem: for α ∈ {-3, -3+2i, -2+3i} there is no Collatz map at modulus (1+i)^3, whatever the table: π^4 + 3 = -1, π^3 - (-3+2i) = 1, π^3 - (-2+3i) = -i are units, so K2 with ε = 1 forces D = U, which commutes, and G26 excludes it. P2 gives a direct proof by explicit literal cycles of the unit table: -3: the eight elements of norm 5, 2+i → 1+2i → -1+2i → -2+i → -2-i → -1-2i → 1-2i → 2-i → 2+i, exponent 3 each; -2+3i: 3 → 4-i → 5-2i → -2-3i → -3 → -4+i → -5+2i → 2+3i → 3; -3+2i: sixteen elements, the unit multiples of 3, 4+i, 5+2i, 2-3i. Status: paper, not in Lean. The research file records only that the remaining table has a cycle through an element of norm 5 or 9; the cycles in P2's proof are not in a research file, and were checked by direct iteration while compiling this catalogue (they hold). Source: M3 4. Papers: P1 Prop 4.2, 6.6; P2 Thm 2.

G20. Digits of norm at most 100 (39 per class): 10,866,920 tables after pruning (2,719,295 conjugacy orbits), none unresolved at the cap; 1,049,464 pass at 10^4 (262,447 orbits; 162 of size 2, commuting with -1 but not i); by multiplier: 3: 6868 (1717 orbits), 3i: 177792, -3i: 83600, 3+2i: 151348, -3-2i: 35032, 2-3i: 94604, 3-2i: 210108, 2+3i: 166250, -2-3i: 123862; random sample of 30 passing tables per multiplier at 10^6: 244 of 270 hold (3: 11/30, 3i: 25/30, -3i: 29/30, 3-2i: 29/30, others 30/30); some failing table per multiplier first shows only above norm 7000, so 10^4 is too small for a reliable count. No commuting table passes. Status: computed (10^4; sample at 10^6). Source: M3 7; LOG m3search_n100_B1e4.txt, m3search_n100_sample_gstopall_1e6.txt. Papers: P1 4.2; P2 2.2.

G21. α = 3 is the weakest multiplier: no six-smallest table passes, about 0.5% of the norm-100 tables pass at 10^4, and 19 of 30 sampled fail by 10^6; the fixed-point lemma removes no digit of norm ≤ 100, so an impossibility proof would need an argument about longer cycles. Status: computed; the question is open (Q4). Source: M3 7, 8. Papers: P1 4.2, 7 item 2; P2 2.2, 8 item 5.

G22. Norm-9 multipliers ±3i have short orbits (stopping times ≤ 184, excursions ~7 bits); norm-13 multipliers stopping times near 2000 and excursions near 40 bits, consistent with log2(9/16) = -0.83 against log2(13/16) = -0.30 bits per odd step. Status: computed. Source: M3 5. Papers: none.

G23. Free-digit sweeps at (1+i)^3 (associates of 3, 3 ± 2i, digits of norm ≤ 5): 972 maps, all converge at 10^5, 108 with a single nonzero cycle. Status: computed to 10^5. Source: D1 5.1; COL 3.2. Papers: P2 9.

### 9.4 Maps commuting with the units below norm 29

G24. Lemma: odd α with 2^k < N(α) < 2^(k+1) and N(α) < 29 has k ∈ {2, 3, 4} and up to a unit α is 2 ± i (k = 2); 3, 3 ± 2i (k = 3); 4 ± i, 5, 4 ± 3i (k = 4). Status: Lean (`alpha_k3`, `alpha_k4`, `box5`). Source: TH Lemma 5.3. Papers: P2 Lemma 4.3.

G25. The η table: k = 3: α = 3 (ε = -1, v = 4, η = 1), 3+2i (-i, 3, -1), 3-2i (-1, 3, -1); k = 4: 4+i (-1, 4, -i), 4-i (-1, 4, i), 5 (-1, 4, -1), 4+3i (-1, 5, i), 4-3i (i, 5, -i). Status: Lean (by `decide`). Source: TH 5. Papers: P2 Thm 4 proof.

G26. Theorem: a map that commutes with the units, lies inside the window and has N(α) < 29 is not a Collatz map, whatever its digit table. Proof: k = 2 by U4; rotate to the listed α (R5); K2 forces unit digits; at k = 4 impossible (each α ≡ a unit u' mod 4, and 1+2i minus a unit is never divisible by 4); at k = 3 D = U and the unit multiples of the cycles -2-i → 1+2i → 1-2i = i(-2-i) (α = 3), -3 → 1+4i → 5-2i → -3+2i → 3 (3+2i), -3 → 4+i → -5-2i → 2-3i → 3i (3-2i) form invariant sets without units (literal cycles of 8, 8, 16 elements). So among commuting maps inside the window 29 is the least norm a Collatz map can have. Status: Lean (`Expr.no_symmetric_rule_below_29`, `units_mod_pi3`, `Expr.no_rule_of_cycle3`, `Expr.no_rule_k4`, `rotate_to`). Credit: none found; it rests on the forced fixed points (K1). Source: TH Thm 5.4; ST Thm 4. Papers: P1 Prop 4.1 (k = 3 case), 6.6, 6.7 end; P2 Thm 4.

G27. G26 uses no minimality; with smallest digits the 5+2i map and its mirror are the only norm-29 commuting maps that pass to 10^9 (3·10^9 for the 5+2i map). Without the criterion Collatz maps exist at norms 5, 9, 13, 17, 25. Status: paper plus computed. Source: ST Thm 4; TH 5; M3. Papers: P2 4.1.

G28. Commuting maps with non-smallest digits: at (1+i)^3 with the 40 smallest elements of a class per orbit, 120 maps, none passes; at (1+i)^4 with 5 candidates per orbit, 175 maps, 3 pass: the 5+2i map, its mirror, and α = -5-2i with D = U ∪ (5-2i)U (four fixed units). Status: computed to 10^5. Source: ST 7; LOG symtest_G_symnonmin_k3-4.txt, symtest_G_symnonmin_k3_40.txt. Papers: P2 4.1.

G29. α = -5-2i with D = U ∪ (5-2i)U: `gstop` over 1 < N(z) ≤ 10^7, 3,927,012 starts, no failure; longest stopping time 9754 at 1782+243i, mean 53.91 (the same as the 5+2i map), largest excursion 80.35 bits at 2728+1543i. Status: computed to 10^7. Source: ST 7; LOG symtest_k4_a-5-2i_D5-2i_gstop_1e7.txt. Papers: P2 4.1.

G30. Earlier non-smallest commuting digit sets at (1+i)^3 (unit multiples of 2+i, 2-i or 3) give 5 to 14 cycles each. Status: computed to 10^5. Source: D1 5; D2 5.2; COL 3.1. Papers: none.

### 9.5 Modulus (1+i)^3 with the criterion (census)

G31. G_3 (α = 3, D = U) has five cycles up to units at 10^9: 2+i → 1+2i (2 odd steps, 92.3% of starts at 10^6), 5+2i → 2+5i (2, 5.6%), 19+6i → 10+19i → 22+7i → 11+22i → 25+8i (5, 1.0%), its mirror through 6+19i (1.0%), the fixed class of 1 (1, 0.2%, falling to 0.07% at 10^7); valuations (3, 3), (3, 3), (3, 3, 3, 3, 4), and 4 for the class of 1. At 10^6 max steps 269, excursion 7.2 bits; `gstop` over (10^6, 10^9] (392,306,389 starts): max stopping 196, mean 9.17, excursion 7.93. Status: computed to 10^9. Source: D1 6; D2 3.7; COL 3.1, 3.5, 3.6; LOG G3_k3.stop, k345_1e6_rerun.txt. Papers: P2 5.2.

G32. G_{3+2i} has four cycles: the fixed class of 1 (63.4%, v = 3), 3 → 1+4i → 2+5i → 2+3i (4 odd, 22.9%, v 3, 3, 5, 4), 7 → 2+9i → 8+3i → 11+2i (4, 12.8%, v 3, 4, 3, 5), 8+7i → 12+7i → 17+6i → 10+13i (4, 0.8%, v 3, 3, 4, 5); at 10^6 max steps 1474, excursion 28.6; to 10^9 max stopping 2426, mean 22.5, excursion 43.4; G_{3-2i} is its mirror. Status: computed to 10^9. Source: COL 3.1, 3.5, 3.6; LOG G3p2i_k3.stop. Papers: P2 5.2.

G33. G_3's mean factor 9/16 is the square of 3x+1's 3/4, and G_3 resembles 3x+1 on all of Z (more than one cycle). Status: remark. Source: D1 6; COL 3.1. Papers: none.

### 9.6 Modulus (1+i)^4

G34. Smallest digits at (1+i)^4: four tied classes (the unit multiples of 1+2i: pairs {1+2i, 1-2i}, {2+i, -2+i} and images), 16 tables, 2 commuting, 6 conjugacy orbits. All 28 associates × 6 orbits = 168 maps at 10^5: 35 pass; 8 commute (the 5+2i map and its mirror, four associates each); 27 do not: 16 of norm 17 (associates ±1 ± 4i, 4 ± i), 11 of norm 29; none of norm 25. Status: computed to 10^5. Source: ST 3, 5.2; LOG symtest_k4_B1e5.txt. Papers: P2 2.3.

G35. Under the criterion the norm-17 multipliers have two cycles; a non-commuting tie-break removes the second: α = 1+4i and 4+i with D = U ∪ {1-2i, -2+i, -2-i, -1-2i} pass `gstopall` to 10^6. The associates -4 ± i, -3-4i, -4-3i can never pass (K2 with ε = 1 and no unit in the classes of the unit multiples of 1+2i). Status: computed to 10^6; paper. Source: ST 5.2, 7. Papers: P2 2.3.

G36. Norm 29, all 128 smallest-digit tables for the eight associates and conjugates u(5 ± 2i): 48 pass at 10^5 (same at 2·10^4): 5+2i 13, -2+5i 7, -5-2i 1, 2-5i 11, 5-2i 1, 2+5i 9, -5+2i 1, -2-5i 5; 8 commute, 40 do not. Status: computed to 10^5 (`variants29.py`). Source: LOG variants29_1e5.txt, variants29_2e4.txt; INV 2.5.7 (the counts are in no research note; TH 4 names the script). Papers: P2 2.3.

G37. Non-smallest non-commuting sample at (1+i)^4: 80 random tables (digits of norm ≤ 25 in their classes, random associates): 12 pass, at norms 17 (7), 25 (3) and 29 (2); norm 25 now has passing maps (-3+4i, 3+4i, 4-3i). Free-digit sweep (norm ≤ 5): 448 maps, 384 converge (all failures α = ±5, ±5i), 76 with a single nonzero cycle. Status: computed to 10^5. Source: ST 8 item 1; LOG symtest_k4_nonmin_sample.txt; D1 5.1; COL 3.2. Papers: P2 2.3, 9.

G38. Commuting census at (1+i)^4, digit set U ∪ (2+i)U (the other gives mirrors): 4+i: 2 cycles (1, 2+i, both 1 odd step); 4-i: 2; 5: at least 35 and orbits that appear to diverge; 4+3i: 3 (1, 6+3i of 11 odd steps); 4-3i: 6 (1, 4+i of 12); 5+2i: 1; 5-2i: 3 (1, 3+4i of 6, 15+16i of 70). Counts the same at 10^5 except α = 5. Basin shares at 10^6: 4 ± i 96.7% / 3.3% or 53.6% / 46.4% by digit set; α = 5: 329,124 starts escaped (10.5%), max excursion 25.9; 4 ± 3i 58.2, 41.0, 0.8% or 34.0, 31.7, 16.1, 9.2, 6.9%; 5-2i 77.9, 18.2, 3.9% (the 70-cycle). Census figures for the 5+2i map at 10^6: max total 6365 steps, max excursion 74.6 bits; for (5+2i, D̄) (3 cycles): 6659, 90.5. Status: computed to 10^6. Source: D1 5; D2 5.2; LOG k345_1e6_rerun.txt (basins and figures only in the log). Papers: P2 Table 5.2, 4.2.

### 9.7 Modulus (1+i)^5

G39. Commuting census at (1+i)^5 (unique smallest table): 6+i: 2 (1, 5 of 4 odd steps); 5+4i: 4 (1, 2+i of 1); 6+3i: 3 (1, 2+i of 2); 7: 18 (1, 2+i of 1); 7+2i: 4 (1, 2+i of 3); 6+5i: 3 (1, 2+i of 7, 164+199i of 130); mirrors 6-i, 5-4i, 6-3i, 7-2i, 6-5i; α = 7 its own mirror; same at 10^5. The map 6+5i (drift log2(61/64) = -0.07 bits) has excursions of 117.8 bits and orbits of 27,206 steps of T below 10^6. Status: computed to 10^6. Source: D1 5; D2 5.2; COL 3.3; LOG k345_1e6_rerun.txt. Papers: P2 Table 5.2, 5.2.

G40. Proposition: among the census maps at (1+i)^3 to (1+i)^5, every map other than the 5+2i map and its mirror has at least two cycles up to units, by exhibited cycles verified in exact arithmetic. Status: computer-assisted, not in Lean. Source: D2 Prop 5; D1 Prop 4. Papers: P2 Prop 5.1.

### 9.8 Modulus (1+i)^6

G41. 28 multipliers × 4 digit sets A to D = 112 maps, 56 up to mirroring, none its own mirror; 22 single-cycle at 10^6: α = 7 ± 4i, 8 ± 3i, 9 ± 2i, 10 ± i, 10 ± 3i, 11 ± 2i. The other 45 up to mirroring: 2 cycles (19 maps), 3 (4), 4 (3), 5 (5), 6 (1), 7 (1), 8 (4), 9 (2), 10 (2); α = 9: 23, 25; α = 11: 560, 568 (181, 211 at 10^5; at least 639 for set A, M16); for 10 ± 5i and two 11 ± 2i maps some starts hit the cap 50,000 (lower bounds). Status: computed to 10^6. Source: D1 5; D2 5.3; COL 3.3. Papers: P2 5.3.

G42. α = 11 ± 2i (norm 125, drift -0.034 bits): excursions pass gcz's 2^200 escape; checked by `gstop` from norm 1 to 10^6 with escape 2^110; the 26 starts that escaped that were iterated in unbounded arithmetic until below their norm, up to 138,239 steps of T and excursions up to about 2^300; single cycle to 10^6. Status: computed to 10^6. Source: D1 5; D2 5.3, 7; COL 3.3. Papers: P2 5.3, 5.4.

G43. Non-commuting sample at (1+i)^6: 6 random conjugacy classes of tie-breaks per associate, 672 of 7840 maps (112 × 70), cap 50,000: 146 pass at 10^5; by norm 65: 38/96, 73: 34/48, 81: 0/24, 85: 19/96, 89: 0/48, 97: 0/48, 101: 26/48, 109: 23/48, 113: 0/48, 117: 6/48, 121: 0/24, 125: 0/96 (inconclusive, 90 of 96 past gcz's 2^100); passing norm-65 maps all associates of 7 ± 4i; associates of 8 ± i cannot pass; every sampled α = 9 or 11 map had escaping starts; one sampled map has three trivial cycles. Status: computed to 10^5. Source: ST 5.3, 4; LOG symtest_k6_sample_B1e5.txt. Papers: P2 9.

### 9.9 Modulus (1+i)^7

G44. 64 odd classes in 16 orbits, unique smallest table closed under conjugation, 46 maps, 24 up to mirroring (13, 15 self-mirror); every start ≤ 10^6 reached a cycle, no cap, no escape; largest excursion 2^153.7 (14 ± 7i). Status: computed to 10^6. Source: D2 5.4; COL 5 items 1, 5. Papers: P2 5.3.

G45. Single-cycle maps at (1+i)^7: 11 ± 4i (137), 12 ± 3i (153), 11 ± 8i (185), 12 ± 7i (193); in each the cycle is the fixed class of 1, one odd step, 8 steps of T (9 for 12 ± 3i); `gstop` for all eight from norm 1 to 10^6 (392,686 starts each), then for -11+4i, -12+3i, -11+8i, -12+7i over (10^6, 10^9] (392,306,389 starts each, no failure, 9 min 39 s); mean stopping times 17.0, 18.4, 28.4, 30.6; longest 284, 512, 1596, 2243; largest excursions 9.7, 14.8, 29.2, 37.4 bits; below 10^6 all below 2^25. Status: computed to 10^9. Source: D2 5.4, 6.4, 7; COL 5 items 4, 6; LOG k7_single_gstop_1e6.txt, k7_single_gstop_1e9.txt. Papers: P2 5.3, 5.4.

G46. Cycle counts of the other 20 maps at (1+i)^7 up to mirroring: 2 (12 ± i); 3 for nine (10 ± 9i, 13 ± 4i, 14 ± 3i, 13 ± 6i, 14 ± 5i, 12 ± 9i, 13 ± 8i, 15 ± 4i, 14 ± 7i); 4 (13 ± 2i); 5 (11 ± 6i, 14 ± i); 8 (12 ± 5i); 11 (10 ± 7i); 16 (9 ± 8i); 33 (15 ± 2i); 74 (15); 166 (13); 766 (11 ± 10i); conjugates agree. Equal norms, unequal behaviour: 9 ± 8i and 12 ± i (both 145) have 16 and 2 cycles; 12 ± 3i (153) has one. Status: computed (10^6; the many-cycle maps to 10^9, M12). Source: D2 5.4; COL 5 items 7, 9. Papers: P2 5.3.

G47. Long cycles at (1+i)^7: a mirror pair of 537 odd steps for α = 15 through 4081+4664i and 4664+4081i; 21 odd steps (181 T-steps) through 11+4i for 14+i; 75 odd steps (661 T-steps) through 7 ± 4i for 15. Status: computed. Source: D2 5.4; COL 5 item 7. Papers: P2 6.2 (537 only).

### 9.10 Below the window

G48. α = 1+2i (N = 5 < 8) at (1+i)^3 with D = U commutes with the units and is a Collatz map: |S(z)| ≤ (√5|z| + 1)/2^(3/2) < |z| once |z| > 1.69, every odd non-unit has norm at least 5, S(1) = -i, one unit cycle 1 → -i → -1 → i → 1. With α = 1 ± 2i, 2 ± i at (1+i)^3 and D = U every orbit with N(z) ≤ 10^5 reaches a unit within 20 odd steps. These maps are the Gaussian counterpart of the x+1 problem. Status: paper (contraction) and computed to 10^5 (`below_window.py`). Source: D2 3.8; TH 5. Papers: P1 8 item 5; P2 4.1.

G49. Contracting multipliers N(α) < 2^(k+1) by modulus (k = 1 units only; k = 2: 5; k = 3: 5, 9, 13; k = 4 adds 17, 25, 29); at (1+i)^3 with unit digits α ∈ {1, 2+i, 3, 3+2i} up to units and conjugation, drifts ln(5/16) = -1.16, ln(9/16) = -0.58, ln(13/16) = -0.21; G_{2+i} at (1+i)^3 has provable contraction, a single cycle, 59 steps max and excursion 3 bits at 10^4. Status: paper and computed. Source: COL 2, 3.1. Papers: none.

### 9.11 The multiplier 3 and 3x+1 as a restriction

G50. On the real line G_3 (α = 3, D = U at (1+i)^3) is the accelerated 3x+1 map applied to whichever of ±x is 1 mod 4 (a non-real unit gives exponent 1, so the digit is ±1, and an integer divisible by π^3 is divisible by 4); |S(x)| ≤ (3|x|+1)/4 < |x| for |x| > 1, so every real orbit decreases to a unit, and the four other cycles lie off the axes. More generally at modulus (1+i)^k, k ≥ 3, with integer digits on odd integers, |S(x)| < |x| on the axes once |x| exceeds the largest digit. Status: paper. Source: D1 6, Prop 5(1); D2 3.7, Prop 1(1); COL 2. Papers: P1 8 item 4; P2 9.

G51. 3 is inert in Z[i], so multiplication by 3 preserves the real line; split primes such as 2 ± i and 5 ± 2i leave no line invariant. Dividing by (1+i)^2 = 2i turns the real axis into the imaginary one; maps that do not commute with the units keep only the two axes; residue-class digits cannot see the sign of x. Status: paper. Source: D1 7; D2 3.7. Papers: P1 8 item 4; P2 9.

G52. No map with α = 3 both agrees with 3x+1 on the positive integers and lies inside the window: π^k | 3n+1 for every odd n ≥ 1 forces π^k | 10 = -iπ^2(2+i)(2-i), so k ≤ 2, and 9 > 2^(k+1). Status: Lean for k ≤ 2 (`prop5_2`, `prop5_2'`, `pi_cube_not_dvd_ten`); paper for the window step. Source: D1 Prop 5(2); D2 Prop 1(2). Papers: P1 8 item 4; P2 9.

G53. The multiplier 3 falls at (1+i)^3, where no commuting map has a single cycle (smallest or tried non-smallest digits): the single cycle of 3x+1 does not survive at its natural Gaussian modulus. No source treats G_3, its cycles, its axes or G52; nearest relatives restrict to 3x+1 in another sense (Joseph on Z_2; Kohl's Z × Z permutation, Example 3.13). Status: computed; literature negative. Source: D1 5.1; LIT 3.4, 2.8. Papers: P1 8 item 4.

### 9.12 Literature maps on Z[i]

G54. Joseph (Fibonacci Quart. 36(4) (1998) 309-316): z/2 on class 0 mod 2, (3z+β)/2 on β ∈ {1, i, 1+i}; restricts to 3x+1 on Z_2; topologically but not Z_2-linearly conjugate to T × T; chaotic in Devaney's sense; no orbits in Z[i] computed. Kucinski (Scranton preprint, May 2000): Joseph's map on Z[i]; Steiner-type period bound; conjugate subsystems (axes, y = x, via h(x+yi) = y+xi); no cyclic points in the second and fourth quadrants or on two families of lines; exactly 77 cycles of period at most 400 (periods 1, 2, 3, 5, 8, 11, 19, 46, 84, 103). Chamberland's survey Sections 6.4, 6.6. Status: literature (Kucinski read through a text extraction; what she claims about "divergent" orbits not seen). Source: LIT 2.1, 2.2, 3.5. Papers: P1 1.1, 8 item 6; P2 9.

G55. Our runs of Joseph's map: mean change of ln N per step (3/4) ln 9 - ln 4 = +0.26 for α = 3 (contracts only for N(α) < 4^(4/3) = 6.35); of 314,197 starts with N(z) ≤ 10^5, 308,624 (98.2%) had not reached a cycle after 20,000 steps or passed coordinates 2^100; the rest fell into 67 cycles, not compared with Kucinski's 77 (an exhaustive count by period, so consistent). Variants with α = 1 ± 2i (drift -0.18): all 16 small β choices converge at 10^5 and 8 have a single cycle (e.g. β = 1, i, -1+i, one cycle through 1 of 6 steps); with α = 2 ± i, 3 or 4 cycles each. These variants are not in Kucinski. Status: computed to 10^5; heuristic drift. Source: D1 10; COL 2, 3.4; LIT 2.2. Papers: P1 8 item 6; P2 9.

G56. Other Gaussian items: Garg (Zenodo 2026) affine rules with dyadic denominators, not integral on Z[i] (summary only); Aliyev (NNTDM 31(3) 2025) lists a Gaussian version of rational cycles as future work; complex-analytic interpolations of 3x+1 (Letherman-Schleicher-Wood, Dumont-Reiter, Chamberland, Berg-Meinardus) are not integral maps. Status: literature. Source: LIT 2.4, 2.11; COL 1. Papers: P1 8 item 12.

## 10. The 5+2i map

H1. Definition: α = 5+2i, N = 29, modulus π^4 = -4 (classes mod 4), 16 < 29 < 32; D = U ∪ (2+i)U = {±1, ±i, ±(2+i), ±(1-2i)}, the smallest digits closed under units; digit table: z ≡ 1, i, -1, -i, 1+2i, 2+i, -1-2i, 2-i (mod 4) get -1+2i, -2-i, 1-2i, 2+i, -1, -i, 1, i. Status: Lean (`pi_pow_four`, `four_dvd_class`, `four_dvd_iff`, `tableH_reps`, `tableH_distinct`, `tableH_covers`, `digitH_correct`, `digitH_mod`). Source: D2 6.1; LEAN RuleH.lean. Papers: P2 4.2.

H2. The table from one congruence: α ≡ 1+2i mod 4 and (1+2i)^2 ≡ 1, so the digits alternate between units and associates of 2+i, and multiplication by α swaps the two orbits of classes. Where 3x+1 has one digit, this map has eight, because the window puts 29 at (1+i)^4. Status: paper. Source: D2 6.1; D1 4. Papers: P2 4.2.

H3. (5+2i)·1 + (-1+2i) = 4+4i = -π^5, so S(1) = -1, S(-1) = 1, one odd step and five divisions each way. Status: Lean (`H_cycle`). Source: D2 6.1. Papers: P2 Prop 4.4.

H4. The map divides by π on even inputs and commutes with the units. Status: Lean (`H_div`, `digitH_unit_table`, `digitH_unit`, `unit_mul_mod`, `divPi_unit_mul`, `pi_dvd_unit_mul`, `H_unit`). Source: D2 8; LEAN HSound.lean. Papers: P2 Prop 4.4.

H5. The other smallest commuting digit set D̄ = U ∪ (2-i)U with α = 5-2i gives the mirror; (5+2i, D̄) is a different map, the mirror of (5-2i, D), with three cycles up to units. Status: paper and computed. Source: D2 6.1; G38. Papers: P2 4.2.

H6. Orbits rise and fall as for 3x+1: 3 → -4-i → 4+3i → -3+2i → 2-3i → -4+3i → -2-i → -i, norms 9, 17, 25, 13, 13, 25, 5, 1, exponents 4, 4, 6, 5, 4, 7, 7; 6827+4464i takes 2042 odd steps (11271 steps of T) to reach a unit, with an excursion of 97.67 bits. Status: computed. Source: D1 4; D2 6.2; LOG alpha5p2i_k4.stop. Papers: P2 4.2.

H7. Conjecture 1 (the 5+2i problem): every orbit on Z[i] \ {0} reaches ±1 or ±i; equivalently it is a Collatz map with a single cycle up to units, the cycles on Z[i] being {1, -1} and {i, -i}. By R5 and R6 it holds for one member of the family (R11) iff for all eight. Status: open. Source: D1 4; D2 1. Papers: P2 4.2.

H8. Verification to 3·10^9: `gcz` classified every N(z) ≤ 10^7 and found only the cycles through ±1, ±i; `gstop` checked every odd z with a > 0, b ≥ 0, 10^7 < N(z) ≤ 3·10^9 in 30 chunks, 1,174,170,307 starts (the sum of the chunk counts in the log), each falling below its own norm, no other cycle, about 15 minutes on one core; coordinates below 2^80; norm comparisons exact below 10^36. Status: computer-assisted (with H9). Source: D2 7; COL 3.6; LOG alpha5p2i_k4.stop. Papers: P2 4.2.

H9. Soundness: if every odd z with a > 0, b ≥ 0 and N_0 < N(z) ≤ B has an iterate of smaller norm, every z with N(z) ≤ B has an iterate of norm ≤ N_0; with N_0 = 10^7, B = 3·10^9 and the gcz classification, every nonzero z with N(z) ≤ 3·10^9 reaches ±1 or ±i. Status: Lean (`H_sound`, an application of `drop_below_start_sound` with `H_div`, `H_unit`). Trusted: the runs, the C programs, 128-bit arithmetic, the compiler, and agreement of `alpha5p2i_k4.rule` with `digitH`, checked entry by entry. Source: D2 7, 8. Papers: P2 Thm 4.5, 7.3.

H10. Statistics over the check: longest stopping time 13913 steps of T at 6742+34011i (chunk (1.2·10^9, 1.3·10^9]); mean stopping time 53.9 (53.82 to 53.93 per chunk); record excursion 122.9 bits (122.85) at 17467+5226i; to 10^9 the longest stopping time is 12226. Status: computed to 3·10^9. Source: D2 7, 6.4; LOG alpha5p2i_k4.stop. Papers: P2 4.2, 5.4.

H11. Drift: at least four divisions, on average five, against 29: log2 29 - 5 = -0.142 bits per odd step, against -0.83 for 3x+1 on x^2, so it contracts about six times more slowly. Status: heuristic. Source: D1 4; D2 6.4. Papers: P2 4.3.

H12. Random-walk model: log2 N walks with drift -0.142 and variance 2 per odd step; 4·10^6 simulated walks give mean stopping time 53.8 steps of T against 53.82 to 53.93 measured. Status: heuristic, matched. Source: D1 9; D2 6.4. Papers: P2 4.3.

H13. Excursion tail: P(climb h bits) ≈ C e^(-θh), θ = 0.165 per bit solving E[e^(θX)] = 1 for X = log2 29 - 4 - G (the diffusion value 2μ/σ^2 = 0.142 is a little low), C ≈ 0.36 (peak just after multiplication); median record over n starts (ln n + ln C - ln ln 2)/θ = 122.4 bits for n = 1.17·10^9 against 122.9 observed. The agreement is closer than the model justifies: the records of 14 of 30 chunks come from orbits through one point of norm about 2^151.2 (checked for five); the mean stopping time is the better test. Status: heuristic; computed for the shared point. Source: D1 9; D2 6.4. Papers: P2 4.3.

H14. Backward form: take the map with all four divisions at once; its inverse branches are w ↦ (1+i)w and w ↦ (-4w - d)/(5+2i), d ∈ D; a quotient that is integral forces z odd with digit d; a non-integer never returns (4 and d prime to 5+2i); so the Gaussian integers in the semigroup orbit of {1, i} are the backward tree, and Conjecture 1 says they are Z[i] \ {0}; {1, i} suffices since -1, -i are images of 1, i; the second branch multiplies norms by 16/29 < 1, so the orbit is not discrete and Erdős-type density arguments do not apply. Status: paper (after Lagarias 2016's form). Source: D2 6.3; LAG 2. Papers: P2 4.4.

H15. Each inverse branch is an affine map of Z^2 (a 3×3 integer matrix, where Klarner, Birget and Satterfield prove freeness undecidable in general) and a 2×2 upper triangular matrix over Z[i] (open for 2×2 upper triangular integer matrices, Cassaigne, Harju and Karhumäki); neither settles freeness here. Coppersmith's feedback criterion might ask whether complements of inverse orbits are unions of classes mod powers of 1+i (a stretch). Status: remark; literature via Lagarias 2016. Source: LAG 2, 3 items 7 to 9. Papers: P2 4.4 (freeness only).

H16. What sets the map apart: it comes first. Every commuting map of smaller norm inside the window fails (G26); the other maps at (1+i)^4 have at least two cycles (G38, G40); it is the largest norm at its modulus, as 7 is at modulus 4 in Z. Among commuting single-cycle maps it has the highest score (0.0280, almost four times the next) and the weakest drift except 11 ± 2i. Status: Lean (G26), computed. Source: D2 5.1, 6.4; SC 3; IC 1 item 6. Papers: P2 5.1, 5.4.

H17. Literature: no source considers α = 5+2i, modulus (1+i)^4 or this digit set, or conjectures a single cycle for a Gaussian map. Status: literature negative with the gaps of F17. Source: LIT 3.3. Papers: none.

## 11. Census and score

S1. Census method: every map that commutes with the units, lies inside the window and has smallest digits, at (1+i)^3 to (1+i)^7, every tie-break included; counted up to rotation and (in parentheses) up to rotation and mirroring; cycles up to units; `gcz` follows every start with N(z) ≤ B to its cycle; every reported cycle is re-iterated by `exact.py` with digits from the congruence before grouping; counts are lower bounds (cycles reached from N(z) ≤ B) except the complete lists of M12. Status: computed. Source: D2 5.1; COL 5 item 10. Papers: P2 5.1.

S2. Summary: (1+i)^3: 3 maps (2), single 0, cycles 5, 4, to 10^9; (1+i)^4: 14 (7), single 2 (1) the 5+2i map, others 2, 2, 3, 3, 6, α = 5 at least 35 and divergent-looking, to 10^6 (5+2i to 3·10^9); (1+i)^5: 11 (6), single 0, cycles 2, 3, 3, 4, 4, 18, to 10^6; (1+i)^6: 112 (56), single 22 (11), others 2 to 10, α = 9: 23, 25, α = 11: 560, 568 (at least 639 for one set), real multipliers divergent-looking, to 10^6; (1+i)^7: 46 (24), single 8 (4), others 2 to 33, α = 13: 166, 15: 74, 11 ± 10i: 766, to 10^6 (single-cycle and M12 maps to 10^9). No commuting map at moduli 1 to (1+i)^2. Status: computed. Source: D2 5.1. Papers: P2 5.1.

S3. Fractions of single-cycle maps: 0/3, 2/14, 0/11, 22/112, 8/46 at (1+i)^3 to (1+i)^7; single-cycle maps are common from (1+i)^6 on. Status: computed. Source: IC 5; COL 5 item 9. Papers: P2 5.1.

S4. Single-cycle drifts: 5+2i -0.142 bits; (1+i)^6 from -0.98 (7 ± 4i) to -0.034 (11 ± 2i); (1+i)^7 -0.90 (137) to -0.41 (193). Status: computed. Source: IC 5; COL 5 item 9; SC 3. Papers: P2 5.4.

S5. Scores of the commuting single-cycle families in Z[i]: 5+2i 0.0280 (drift -0.142); 11+2i 0.0076 (-0.034); 10+3i 0.0065 (-0.232); 10+i 0.0057 (-0.342); 9+2i 0.0039 (-0.591); 12+7i 0.0026 (-0.408); 11+8i 0.0024 (-0.469); 8+3i 0.0019 (-0.810); 12+3i 0.0013 (-0.743); 11+4i 0.0005 (-0.902); 7+4i 0.0002 (-0.978). Comparison: 3x+1 0.167; 7x ± 1 0.107; 2-i at (1+i)^2 0.05; the non-commuting modulus-(1+i)^3 maps 1/72 (norm 9) and 5/104 (norm 13); Eisenstein 5+2ω 1/16 - 1/19 = 0.0099 (computed only in P1). Status: computed. Source: SC 3; M3 1 item 4. Papers: P1 2.3, 5.1; P2 5.4.

S6. Drift predicts excursion size among single-cycle maps: the (1+i)^7 maps reach 9.7 to 37.4 bits and 284 to 2243 steps at 10^9 (G45), the 5+2i map 122.9 bits and 12,226 steps, the 11 ± 2i maps about 300 bits below 10^6. Status: computed. Source: D2 6.4; COL 5 item 6. Papers: P2 5.4.

S7. Drift comparison of first single-cycle maps in three rings (natural log): 7x ± 1 -0.134, 5+2i -0.098, 5+2ω -0.290. Status: heuristic quantities. Source: IC 7. Papers: none.

## 12. Maps with many cycles and best approximations

M1. Maps studied: (1+i)^7 α = 13, 15, 11 ± 10i, 15 ± 2i; (1+i)^6 α = 9, 11 with all four digit sets. No infinite family; no affine map relates two cycles except mirroring; cycles spread over all angles; no shared parity vector beyond most valuations equal to k. Structure tests (`famstruct.py`): only conjugation composed with a unit relates cycles (13: 79 mirror pairs among 160, 2 self-mirror; 15: 16 pairs in the 43-class); no equal consecutive differences; excess V - kn of 4 in 10 entries (13), 26 in 33 (11+10i); 13's 160 cycles use 35 necklaces, 11+10i's 728 all distinct; composite classes (18 of 29 66-cycles of 11+10i split as 257 + 257; the two 20-cycles of 13 as 73 + 75; the 75-cycles of 15 contain a 43-block of sum 336). Status: computed. Source: FAM 1, 4. Papers: P2 6.1.

M2. Cycle equation: a cycle (n, V, ε) satisfies z_0(επ^V - α^n) = Σ d_j α^(n-1-j) π^(V - s_j); with rel = |1 - α^n/(επ^V)|, N(επ^V - α^n) = 2^V rel^2 and |z_0| is of order 1/rel, so large cycles need α^n close to επ^V; for real α one condition (V/(2n) ≈ log2 α), for non-real α two (V/n ≈ log2 N(α) and n arg α - Vπ/4 - arg ε ≈ 0). Status: paper. Credit: on Z classical: Böhm and Sontacchi's rational point of a parity vector, Lagarias 1990's BSL theorem (each parity vector gives a unique rational periodic point) and count I(n), Steiner 1977, Eliahou 1993, Crandall 1978, Simons and de Weger 2005, Belaga and Mignotte, Wegner arXiv:2101.08060 (cycles of 3x+k through 2^a - 3^b); the angle condition and the form for R were not found (IAP row 9). Source: FAM 3. Papers: P1 6.7; P2 6.1.

M3. Near-coincidences and main classes (class, cycles, E): 13: 13^10 = 1.00305·2^37, rel 3.05·10^-3, (10, 74, -i), 160, 117; 15: 15^43 = 0.9974·2^168, 2.56·10^-3, (43, 336, -1), 32, 29.3; 11+10i: (11+10i)^33 near i π^257, 6.17·10^-4, (33, 257, i), 728, 764; 15+2i: (15+2i)^6 near iπ^47, 1.58·10^-2, (6, 47, i), 28, 20.5; 11 at (1+i)^6: 11^37 = 0.99929·2^128, 7.1·10^-4, (37, 256, -1), 498 or 481 at 10^6, 613; 9 at (1+i)^6: 9^6 = 1.0136·2^19, 1.36·10^-2, (6, 38, i), 15 or 17, 18.4. Status: computed; E heuristic. Source: FAM 1, 3. Papers: P1 4.2 (11+10i); P2 6.1.

M4. Full class distributions: 13: (10, 74, -i) 160, (3, 22, -i) 2, (20, 148, -1) 2, fixed 1 (1, 8), (2, 14, i) through 1+2i; 11+10i: (33, 257, i) 728, (66, 514, -1) 29, (132, 1028, 1) 3, (99, 771, -i) 2, (198, 1542, -1) 1, (264, 2056, 1) 1, (1, 7) and (2, 16) one each; 15: (43, 336, -1) 32, (1, 8, -1) 16, (32, 250, -i) 7, (75, 586, i) 6, (247, 1930, i) 4, (107, 836, 1) 2, (11, 86, -i) 2, (8, 62, i) 2, (537, 4196, 1) 2, (10, 78, i) 1; 15+2i: (6, 47, i) 28, (1, 8, -1) 3, (5, 39, -i) 1, (12, 94, -1) 1; 9: (6, 38, i) 15 or 17, (1, 6, -i) 8; 11 at (1+i)^6: (37, 256, -1) 498 or 481, (74, 512, 1) 46 or 51, (2, 14, i) 7, a few each of (13, 90), (111, 768), (148, 1024), (11, 76), (185, 1280), (222, 1536), (209, 1446). Status: computed. Source: FAM 2. Papers: none.

M5. Smaller classes sit at multiples and sums of resonant lengths: for 15, 75 = 43 + 32, 107 = 75 + 32, 247 = 5·43 + 32, 537 = 12·43 + 21; for 11+10i, 66 and 132, with rel about m times the value at 33. Status: observation. Source: FAM 2, 3. Papers: P2 6.1.

M6. Heuristic count: E(n, V, ε) = 2^((k-1)n) C(V - kn + n - 1, n - 1)/(4n N(επ^V - α^n)), from the 2^((k-1)n) classes of odd π-adics with given valuations, each with one solution of S^n(z) = εz, integral with probability 1/N(επ^V - α^n); it matches main classes within a factor of about 1.4; further rows: 13 (20, 148, -1) 2.8 vs 2; 11+10i (66, 514) 44.6 vs 29, (99, 771) 7.1 vs 2, (132, 1028) 1.7 vs 3; 15 (32, 250) 1.6 vs 7. Status: heuristic. Credit: not found (IAP row 9; Lagarias 1990 unread, may contain such a heuristic). Source: FAM 3; LOG families_theory_classes.txt. Papers: P1 6.7 (Z form, Z20); P2 6.1.

M7. Totals of E over n ≤ 20000, |V - n log2 N| ≤ 3, all ε: 129 (13), 70 (15), 30.5 (15+2i), 825 (11+10i), 31 (9), 721 (11 at (1+i)^6); partial sums at n = 100, 300, 1000, 3000, 20000 constant from n = 1000 (3000 for 11; e.g. 11: 687.9, 707.9, 718.2, 721.2, 721.2); convergence by large deviations (V/n near log2 N(α) below the mean k+1); the heuristic predicts finitely many cycles with totals near the counts. Status: heuristic. Source: FAM 3; LOG families_heur.txt. Papers: P2 6.1.

M8. For 11+10i both conditions hold at n = 33 to about 6·10^-4, far better than the generic simultaneous-approximation bound of order n^(-1/2); classes at multiples have E falling 764, 44.6, 7.1, 1.7, 0.5; observed counts along multiples m = 1 to 8: 728 (E 764), 29, 2, 3, 0, 1, 1. No parametrized family with fixed n can exist (each class is finite by M9). Status: heuristic and computed. Source: FAM 3, 5. Papers: P2 6.1.

M9. Lemma (bound on one element): with g_j = ln|α| - (v_j/2) ln 2, c = max(0, ln r), G_m = min(m(ln|α| - (k/2) ln 2), c) for m < n and G_n = ln r, some element has |z| ≤ (d_max/(|α| rel)) Σ_{m=1}^n e^(G_m) (start at a prefix-minimum index, the cycle lemma). Status: paper, not in Lean. Source: FAM 5; D2 Lemma 1. Papers: P2 Lemma 6.1.

M10. Lemma: if |V - n log2 N(α)| ≥ 3 the bound of M9 is at most 1.55 d_max n/|α|, so per n only six V and four units need evaluating; d_max = 7 at (1+i)^7, 5 at (1+i)^6. Status: paper. Source: FAM 5. Papers: P2 Lemma 6.2.

M11. Coverage of M9: 13: n ≤ 156 at 1.3·10^7, n ≤ 216 at 10^9 (217 needs 1.4·10^9); 15: n ≤ 246 at 10^9 (247: 7.1·10^9); 15+2i: n ≤ 982 at 2.85·10^7, n ≤ 1498 at 10^9; 11+10i: all n ≤ 2000 (largest 6.4·10^8, about 6.3·10^8 along n = 33m); 11 at (1+i)^6: n ≤ 578 at 10^9, bound for n = 37 about 5.6·10^8. Ratio least norm / bound at most 0.29 (13), 0.27 (11+10i), 0.51 (15+2i), 1.0 for 15 at the fixed point 7 where the bound is attained. Status: computed (double precision, margin ≥ 1.4). Source: FAM 5; LOG families_cover.txt. Papers: P2 6.3 (partly).

M12. Theorem (complete lists): at (1+i)^7 with the smallest digit set, α = 13 has exactly 166 cycles up to units with least norm ≤ 10^9, including every cycle with ≤ 216 odd steps, 160 of 10 steps; α = 15 has 74, every cycle ≤ 246, 32 of 43; α = 11 ± 10i has 766, every cycle ≤ 2000, 728 of 33, 29 of 66, 2 of 99, 3 of 132; α = 15 ± 2i has 33, every cycle ≤ 1498, 28 of 6. Status: computer-assisted. Source: FAM 5; D2 Thm 1. Papers: P2 Thm 5.

M13. Proof runs for M12: gcz to 1.3·10^7, 1.65·10^7, 10^7, 2.85·10^7 (cycle limit raised to 65536 in a scratchpad copy `gczbig`); gstop to 10^9: 13 over (1.3·10^7, 10^9], 387,593,990 starts, no cycle, 128 s; 11+10i over (10^7, 10^9], 388,772,062 starts, one cycle with least element 2318+2599i (norm 1.21·10^7, already known), 329 s; 15 over (1.65·10^7, 10^9], 386,219,564 starts, 8 capped (311+4664i, 326+4891i, 359+5382i, 2679+3062i and mirrors), each reaching the 537-step pair within 1 to 21 odd steps without dropping, 370 s; 15+2i over (2.85·10^7, 10^9], 381,507,118 starts, no cycle, 419 s. Stopping statistics: 13 max 841, mean 21.48, excursion 21.26 bits; 11+10i 9518, 55.54, 74.86; 15 21409, 62.90, 84.06; 15+2i 13931, 72.12, 92.10. Status: computer-assisted / computed to 10^9. Source: FAM 5; LOG families_gstop_*, families_gstop_k7_summary_1e9.txt. Papers: P2 Thm 5 proof (runs; statistics none).

M14. Exact verification (`famverify.py`): for every listed cycle the digits (1+i)^(v_j) z_(j+1) - αz_j lie in D and are the ones the map picks, elements are odd, the cycle equation holds, elements are pairwise non-associate; 166, 74, 33, 766, 4·(23 to 25) and 4·(560 to 568) cycles, none failed; rel^2 = N(den)/2^V exactly 9.318·10^-6, 3.808·10^-7, 6.576·10^-6, 2.503·10^-4. Status: computed exactly. Source: FAM 5. Papers: P2 Thm 5 proof.

M15. Saturation: cycles with least norm ≤ B (B = 10^2 to 10^8): 13: 2, 6, 30, 127, 166, 166, 166; 15: 20, 29, 40, 62, 72, 72, 74; 15+2i: 5, 21, 33, ...; 11+10i: 2, 3, 6, 66, 395, 765, 766; 9 (set A): 9, 13, 23, 23, 23; 11 (set A): 8, 11, 26, 98, 445, 568; largest least norms 2.7·10^5 (13), 1.2·10^7 (11+10i), 3.84·10^7 (15, the 537 pair). Status: computed. Source: FAM 2. Papers: P2 6.3 (11+10i, 13).

M16. α = 11 at (1+i)^6 has no complete list: gstop over least norms (10^6, 3·10^6] for set A found 172 cycles, 101 already known, so at least 639 (same for the mirror); out of reach because many starts fall into cycles above their own norm and run to the cap (about 13,000 starts per second). Status: computed. Source: FAM 2; LOG families_gstop_k6_a11_A_1e6_3e6.txt. Papers: P2 6.3.

M17. Expected missing cycles beyond n = 300: about 1.4 for 15 (the 537 pair shows such cycles occur), below 0.1 for the others, about 13 for 11 at (1+i)^6. Status: heuristic. Source: FAM 5. Papers: none.

M18. Angles: cycle elements spread over all angles (10-degree bins 2599 to 2715 for 11+10i's 33-cycles, 156 to 188 for 13's 10-cycles); share within relative distance 0.02 of an axis or diagonal 0.049, 0.054, 0.065, 0.051 against 0.051 uniform; 15 bunches near 0.066. Status: computed. Source: FAM 2. Papers: P2 6.1 (spread only).

M19. Records: best(n) = min over V, ε of rel; n is a record if best(n) < best(m) for all m < n; for real α the records are convergent denominators of log2|α| plus occasional small-n extras (n = 2 for 5, n = 3 for m = 19); for complex α best simultaneous approximations. Status: definition. Source: CONV preamble. Papers: P2 6.2.

M20. The largest class always sits at a record, and so does every class with ≥ 3 cycles except multiples and sums of the main record (74, 111, 148 for 11; 66, 132 for 11+10i; 75 for 15); dominant lengths for real α: 10 (13); 32, 43, 247, 537 (15); 37 and 2, 11, 13, 24 (11 at (1+i)^6); 6 (9); 3, 28 (5 at (1+i)^4); 5, 26 (7 at (1+i)^5). Every convergent denominator of log2 15 up to 537 except 290 carries cycles. Status: computed. Source: CONV 1, 4.1; LOG convergents_families.txt. Papers: P2 6.2.

M21. Records up to n = 3000 (best, E, cycles): 13: 1 (0.19, 2.6, 1), 3 (0.073, 3.2, 2), 7 (0.065, 0.76, 0), 10 (0.0031, 117, 160), 227 (1.3·10^-4, 2·10^-10, 0); 15: 1, 10, 11, 32, 43, 247, 290 (0.0012, 0.72, 0), 537 (1.7·10^-4, 1.3, 2); 11+10i: 1, 2, 14, 19, 33; 15+2i: 1, 6, 1505; 9: 1, 5, 6, 47, 53, 153, 665; 11 at (1+i)^6: 1, 2, 11, 13, 24, 37, 949 (E 2.9), 986 (5.2), 1935 (2.0); 5 at (1+i)^4: 1, 2, 3 (29, 28), 28 (2.3, 3), 59 (1.2, 1), 146; 7 at (1+i)^5: 1, 5, 26, 109; 10+7i at (1+i)^7: 1, 4, 9, 616. Continued fractions: log2 13 = [3; 1, 2, 2, 1, 22, 23, ...]; log2 15 = [3; 1, 9, 1, 2, 1, 5, 1, ...]; log2 11 = [3; 2, 5, 1, 1, 1, 25, ...]; log2 5 = [2; 3, 9, 2, 2, ...]. Status: computed. Source: CONV 4.1. Papers: P2 6.2 (continued fractions and denominators).

M22. Census-wide: 54 maps (one per mirror pair, ≥ 3 cycle classes, (1+i)^4 to (1+i)^7) rerun at 10^6: 116 classes with n ≥ 2, 67 at records holding 1909 of 2104 cycles; in every class with ≥ 3 cycles (V, ε) is the best pair; examples n = 2 for 9+4i (4 or 5 per digit set) and 12+5i (7), n = 3 for 7+2i and 11+6i, n = 5 for 13+2i, n = 11 for 4+3i at (1+i)^4, n = 16 for 9+6i. Status: computed (`convrun.py`; outputs in a scratchpad only). Source: CONV 3, 4.2; LOG convergents_census.txt. Papers: P2 6.2.

M23. The converse fails: records without cycles are common (13 at 7, 227; 9 at 5, 47, 53, 153; 15 at 290), mostly with small E; only two records with E ≥ 1 and no cycle (n = 2 for 6+5i at (1+i)^5, E = 1.0; n = 16 for one digit set of 9+6i, E = 1.3), consistent with Poisson fluctuation. Sporadic cycles at non-record lengths: 8+7i at (1+i)^6, n = 82, rel 0.10, E = 0.005, 79% of the basin; the 70-cycle of 5-2i at (1+i)^4, rel 0.07, E = 0.018; 10+3i at (1+i)^6, n = 70, E = 0.009. Status: computed. Source: CONV 1, 6. Papers: P2 6.2 (converse only).

M24. Simultaneous against modulus-only approximation: 9+4i (log2 97 = [6; 1, 1, 2, 458, ...]; n = 5 modulus error 0.0004, angle error 0.34 in units of π/4, no 5-cycles; cycles at n = 2, angle error 0.065); 12+5i (n = 5 angle 0.49, none; 7 cycles at n = 2, angle 0.005); 13+2i (modulus records 7, 23 with angle 0.64, 0.53, none; full record 5 has 3); 11+10i (n = 33 both 0.0008 and 0.0007; modulus records 4, 5 with bad angles, none). Status: computed. Source: CONV 4.3; LOG convergents_modonly.txt. Papers: P2 6.2.

M25. The record property follows from E: if E rel^2 decreases in n, the n maximizing E is a record; records are a corollary of the heuristic, not an independent principle. Status: heuristic argument. Source: CONV 1, 7. Papers: P2 6.2.

M26. Prior art for convergents: cycles of 3x+1 are constrained by convergents of log2 3 (Crandall 1978, Steiner 1977, Eliahou 1993, Halbeisen-Hungerbühler 1997, Simons-de Weger 2005: m-cycles ruled out for m ≤ 68); 3x+q with q = 2^r - 3^s; a 34-step cycle of 7x+11; Belaga and Mignotte on 3x+d (bounds d^(k^c), polynomial bound for fixed length; tables of 2^l - 3^k); Lagarias 1990 integer cycles of 3x+k for infinitely many k. The empirical statement that cycles of a many-cycle map concentrate at best-approximation lengths with class sizes set by rel was not found. Status: literature. Source: CONV 2, 7; IAP 3.9. Papers: P1 6.7; P2 6.2.

M27. Prediction: α = 11 at (1+i)^6 should have a few cycles of 949 and 986 odd steps (E = 2.9, 5.2) near norm 10^13, out of reach. Status: heuristic prediction. Source: CONV 4.1, 7. Papers: P2 6.3.

## 13. Divergence on invariant lines

L1. Among commuting smallest-digit maps, orbits that appear to diverge occur only for the real multipliers 5 at (1+i)^4 and 9, 11 at (1+i)^6; none for non-real α; integer maps in the window showed none. Every digit set tried with α = 5, 9, 11 had such orbits. Status: computed. Source: D2 5.5; D1 8; ST 5.3. Papers: P1 4.2; P2 6.4.

L2. α = 5: of about 1500 random starts with norm up to 1.8·10^7, 176 appeared to diverge, all on x - y = ±1 or x + y = ±1. (1+i)^6: about 150 random starts with |x|, |y| ≤ 1000 per digit set, escaped past norm 2^120: α = 9: 16 of 152 (A), 13 of 142 (B), 16 of 147 (C), 29 of 160 (D); α = 11: 68 of 145, 69 of 156, 65 of 147, 55 of 147; all on the eight lines, all but 13 on diagonals, valuations k, k+1, k+2 only. Status: computed. Source: D1 8; COL 3.7; FAM 6; LOG families_k6_escape.txt. Papers: P2 6.4.

L3. Proposition: the eight lines L: x = ±1, y = ±1, x - y = ±1, x + y = ±1 are mapped into L by S for α = 3 ((1+i)^3), 5 ((1+i)^4, both digit sets), 7 ((1+i)^5), 9, 11 ((1+i)^6, all four), 13, 15 ((1+i)^7), with exponents k, k+1, k+2. Argument: z = u + tw, the transverse coordinate of αz + d is that of αu + d, which depends only on t mod a power of 2 while the exponent stays bounded; checked over windows of 2^16 and 2^18 consecutive t from 10^12 on two lines, the other six by units. Status: computer-assisted (residue period check). Source: FAM 6 Prop 3. Papers: P1 4.2; P2 Prop 6.3.

L4. At odd k an odd exponent turns an axis line into a diagonal one and back, so with the axis lines included every point of L maps into L (replacing the old 876 of 1000 sample points of the diagonals). Status: paper. Source: FAM 6. Papers: P2 6.4.

L5. Drift along L from a two-state chain (axis, diagonal), stationary distribution over a full period (mean v on axis / diagonal lines, share of axis, mean v, log2 N, drift in bits of |z| per odd step): 3: 3.750/3.500, 0.400, 3.600, 3.170, -0.215, converges; 5: 4.500/4.375, 0.200, 4.400, 4.644, +0.122, diverges; 7: 6.125/5.500, 0.444, 5.778, 5.615, -0.082, converges; 9: 6.500/6.188, 0.111, 6.222, 6.340, +0.059, diverges; 11: 6.500/6.563, 0.273, 6.545, 6.919, +0.187, diverges; 13: 7.938/7.500, 0.421, 7.684, 7.401, -0.142, converges; 15: 8.313/7.500, 0.471, 7.882, 7.814, -0.034, converges; independent of the digit set; the sign matches observation in every case. Status: heuristic (equidistribution of t residues). Source: FAM 6; LOG families_line_drift.txt. Papers: P1 4.2; P2 6.4.

L6. For α = 15 a uniform average over L would give +0.021, but the chain spends only 0.47 of its time on axis lines; exact iteration: 300 starts on L (t up to 2·10^9) for 15 all returned, mostly to the cycle through 7+8i; 200 starts on L for α = 9 all escaped (`famlinerun.py`). Status: computed. Source: FAM 6. Papers: P2 6.4.

L7. The (1+i)^7 real maps show no divergence because their drift along L is negative; (1+i)^7 = 8(1-i), so α/(1+i)^k is never real up to a unit for odd k. Status: heuristic and paper. Source: FAM 6; COL 5 item 8. Papers: none.

L8. The lines carry the divergence, not the cycles: for real α a cycle meets L in all or none of its elements; 13: 6 of 160 10-cycles on L; 15: 2 of the 8-cycles and 2 of the 32-cycles; 11+10i: 37 of 728 cycles have some elements on L by chance, only the 2-cycle through 1+2i lies on L; 11 at (1+i)^6: no 37-cycle meets L; 9: 4 or 6 of 15 or 17 6-cycles entirely on L. Status: computed. Source: FAM 4, 6; LOG families_lines.txt. Papers: P2 6.4.

L9. Z has no analogue: the line is the whole ring, and every integer map in the window has negative drift. Status: remark. Source: IC 4.2. Papers: P1 4.2.

L10. Eisenstein: for α = 5 at modulus 4, 59 of 139 random starts with |a|, |b| ≤ 300 passed norm 2^400, all on b = 1 (33) or a - b = 1 (26) in the sector 0 ≤ b < a; for α = 9 at modulus 8, 45 of 139, on the same two lines. Status: computed. Source: IC 7; LOG zeis_lines.txt. Papers: P1 5.1.

L11. Precedents: MW 1985 Example 5 (even and odd integers as two invariant sets, products (3/4)^(1/2) < 1 and (7/4)^(1/2) > 1, even starts cycle, odd starts mostly diverge), the earliest found; Leigh 1986 Example 2 (two positive classes, one with weighted product √3/2 < 1); Matthews 2010 Example 10.1 on Z[√2] (real multiplier 3, three invariant one-dimensional sets, divergent trajectories evenly distributed mod (√2)^a) and 10.3; Kucinski's invariant subsystems of Joseph's map; Kionke's tame cones for scalar multipliers. The specific lines and the drift values were not found. Status: literature. Source: IAP 3.10; LEI 3.10; MW 4.10; LIT 2.5, 2.6. Papers: P1 6.7; P2 6.4.

## 14. Other rings

### 14.1 The Eisenstein integers

E1. Setting: z = a + bω, ω^2 = -1 - ω, N = a^2 - ab + b^2; π = 2 inert, q = 4; z odd iff a, b not both even; three odd classes mod 2; six units ±1, ±ω, ±ω^2; κ = 1; canonical representative 0 ≤ b < a; window (4^k, 4^(k+1/3)); modulus 2 holds no multiplier ((4, 6.35) holds only 5, not a norm). Status: paper. Source: IC 7; ST 10. Papers: P1 5.1, Prop 6.1.

E2. Modulus 4: 12 odd classes in 2 orbits under the units, unique smallest table U ∪ (1-ω)U (commuting); window (16, 25.40) holds the odd norms 19, 21, 25 and five multipliers up to units: 5+2ω, 5+3ω (19), 5+ω, 5+4ω (21), 5 (25); three maps up to units and conjugation. Status: paper and computed. Source: IC 7. Papers: P1 5.1.

E3. At N(z) ≤ 10^5: 5+2ω (and conjugate 5+3ω): drift -0.419 bits (-0.290 ln), 1 cycle, the fixed class of 1; 5+ω: -0.274 bits, 3 cycles (1, 2+ω both fixed, 7+3ω of 11 odd steps); 5: -0.023 bits, 84 cycles found and 9104 starts unresolved. Status: computed to 10^5. Source: IC 7; LOG zeis_k2_B1e5.txt. Papers: P1 5.1.

E4. For 5+2ω and 5+3ω every odd z with 1 < N(z) ≤ 10^6 reaches a smaller norm in exact arithmetic (one per associate class; log max 96), so every odd z with N(z) ≤ 10^6 reaches a unit: the first modulus that allows commuting maps already has a single-cycle (Collatz to 10^6) map, the smallest multiplier there, unlike Z[i]; this attributes the delay in Z[i] to κ = e = 2, not to the dimension. Status: computed to 10^6. Source: IC 1 item 7, 7; LOG zeis_k2_drop.txt. Papers: P1 5.1.

E5. Literal check of all six associates at modulus 4: 12 of 30 maps pass at 10^5 (the associates of 5+2ω and 5+3ω); trivial cycles: one through all six units (-2-5ω, 2-3ω, 3+5ω, 5+3ω), two of three units (-5-3ω, -3-5ω, -2+3ω, 2+5ω), three of two units (-5-2ω, -3+2ω), six fixed units (3-2ω, 5+2ω); 8 of the 12 have trivial cycles that move between units; the counts 1, 2, 3, 6 are the divisors of 6, and 3 (impossible for commuting maps in Z[i]) appears. Status: computed to 10^5. Source: ST 10; LOG eistest_k2_min.txt. Papers: P1 5.1, 6.2.

E6. Modulus 8: 48 odd classes in 8 orbits, one tied (D8), two conjugate commuting tables; window (64, 101.6), 16 multipliers up to units, norms 67, 73, 75, 79, 81, 91, 93, 97, 32 maps; none single-cycle at 10^5: mostly 2 or 3 cycles, 4 for two, 8 for 10+5ω, 10 or 14 for norm 73, α = 9: 42 cycles and 5829 unresolved; every map has a cycle through 1 (for 11+5ω, 11+6ω with one digit set a 2-cycle through 1). Status: computed to 10^5. Source: IC 7; LOG zeis_k3_B1e5.txt. Papers: P1 5.1.

E7. Without the criterion: modulus 8 has 64 smallest-digit tables, 2 commuting; a sample of 2 random tie-breaks for each of the 96 associates (192 maps): 18 pass at 10^5, all of norm 91 (associates of 11+5ω, 11+6ω), 14 with moving trivial cycles; modulus 4 non-smallest: of 40 random digit sets (digits of norm ≤ 13), 12 pass (norms 19, 21), 9 with a trivial cycle through two or more units. As in Z[i] at even moduli, a tie lets a non-commuting table succeed where the commuting ones fail. Status: computed to 10^5. Source: ST 10; LOG eistest_k3_min_sample.txt, eistest_k2_nonmin.txt. Papers: P1 5.1 (modulus 8 sample).

E8. The REU slides mention an Eisenstein analogue dividing by the ramified prime 1-ω (q = 3) without computations; our maps divide by the inert 2; the 1-ω version is untested. Status: literature and gap. Source: LIT 2.3; IC 7. Papers: P1 5.1, 7 item 7.

### 14.2 Untested predictions and literature on other rings

O1. Z[√-2]: 2 = -(√-2)^2 ramifies, π = √-2, q = 2, e = 2, U = {±1}; 3 = 1 + 2·1 is a norm, so (2, 4) at modulus π holds ±1 ± √-2 and maps of the shape of 3x+1 exist inside the window (W11); κ = 2, so commuting maps start at π^3 as in Z[i] (U1); conjugation sends π to -π, so the mirror carries (-1)^v and should lose the relation after one step for non-commuting tables. Tests: whether the modulus-π maps are Collatz maps on the whole ring (the forced fixed points apply when π^v - α is a unit); whether the delay of commuting Collatz maps recurs with two units. Status: the structural statements are paper (from W11, U1, R3); the behaviour is an untested prediction. Source: P1 5.2 only (IC 2.3 gives the thresholds). Papers: P1 5.2, 6.1, 6.6, 7 item 7.

O2. Z[(1+√-7)/2]: 2 = θθ̄ splits, θ = (1+√-7)/2, θ and θ̄ not associates; q = 2, e = 1, U = {±1}, κ = 1, so maps dividing by θ that commute with -1 exist from θ^2 as in Z; conjugation moves the prime to the other one, so there is no mirrored map in the sense of R1. Prediction: a map and its conjugate are conjugate maps of two families, one per prime above 2. Status: structural part paper; prediction untested. Source: P1 5.2 only. Papers: P1 5.2, 6.5, 7 item 7.

O3. Z[(1+√-11)/2]: 2 inert, q = 4, e = 1, κ = 1 as in Z[ω], but U = {±1}; window (4^k, 4^(k+1/3)), commuting maps from modulus 4, at most two trivial cycles. Test: if a commuting Collatz map appears at the first modulus with multipliers, the early appearance in Z[ω] is due to q and κ, not to six units. Status: structural part paper; prediction untested. Source: P1 5.2 only. Papers: P1 5.2, 7 item 7.

O4. Z[√2]: infinite unit group (generated by -1 and 1+√2), 2 = (√2)^2 ramifies; no map commutes with all units at any modulus, maps commuting with ±1 exist from (√2)^3 (U2). Leigh's map z/√2, (3z+1)/√2 at modulus √2 lives here (Matthews Example 10.1). The definitions of trivial cycle and W1 must be restated with both real embeddings and the window read with |N|; divergence along invariant lines expected to be common. Status: structural part paper; prediction untested. Source: IC 2.3; P1 5.2. Papers: P1 5.2, 7 item 7.

O5. Literature on rings of integers: Matthews 2010 Section 10 (maps on O_K mod a non-unit; Example 10.1 Z[√2] Leigh's map, Example 10.2 Z[√3] with 103 cycles and one apparently divergent orbit, 10.3 Z[√2] with a unit factor; no general definition, norm condition or conjecture; Section 11 invites systematic study); Leigh is reported by Siegel as the first (1983 in arXiv:2111.07882, 1981 in arXiv:2412.02902), while Leigh's Acta Arith. 46 (1986) 125-143 (received 7 June 1984; dated 1985 by Lagarias and Matthews) is on Z only and does not bear the attribution out; Kionke, Siegel, Siegel-O'Dwyer (F16); polynomial analogues (Nichols arXiv:1610.02545, a Terras-type theorem over F_2[t] and a hyperelliptic ring; Matthews-Leigh 1987; Hicks-Mullen-Yucas-Zavislak 2008; Gallardo-Rahavandrainy; Monks arXiv:2508.10153; Behajaina-Paran, whose map commutes with every constant). Status: literature. Source: LIT 2.6, 2.7, 2.9, 2.10; IAP 2.2, 2.3; LEI 1, 4. Papers: P1 1.1, 5.2, 8 item 11.

O6. Attributions under test: (1) whether a modulus carries multipliers depends on which odd numbers are norms; (2) the exponent of the window depends on q; (3) commuting maps exist exactly from k = κ+1, and smallest digits commute where the lattice gives unique least elements; (4) for commuting maps the number of trivial cycles divides |U|; (5) a non-commuting map is related to its mirror only when the automorphism fixes π. Z[ω] confirms (2), (3), (4) and (5) (the last as a consequence of R7). Status: heuristic framing, partly proved. Source: P1 5 (assembled from W11, W13, U1, T2, R7). Papers: P1 5.

## 15. Verification methods and bounds

V1. Drop-below-start soundness for maps commuting with the units: if every odd z with a > 0, b ≥ 0 and N_0 < N(z) ≤ B has an iterate of smaller norm, every z with N(z) ≤ B has an iterate of norm ≤ N_0. Status: Lean (`drop_below_start_sound`). Source: D2 7. Papers: P2 Thm 4.5.

V2. All-quadrant soundness for any T that divides by π on even inputs, no condition on units, at four times the starts. Status: Lean (`drop_below_start_sound_all`). Source: TH Thm 6.1. Papers: P1 4.2; P2 2.

V3. Programs. `gcz.c`: memoized full classification of every z with N(z) ≤ B, Brent cycle detection, 128-bit coordinates, escape at 2^100, step cap, tables mod 4, 8, 16. `gstop.c`: drop-below-start for commuting maps, one associate per class, exact norm comparisons below 10^36 (above, iteration continues without comparing, so no false drop can be reported), escape 2^110, reports returns to an associate of the start and cap hits. `gstopall.c`: the same over all quadrants, literal cycles. `m3search.c`: product of candidate lists at modulus π^3, checks each digit's congruence, optional reduction mod z ↦ uz. `exact.py`: unbounded arithmetic, digits from the congruence, independent of the C tables. `famverify.py`: M14. Integers: `zcz.c` (census and drop check, 128-bit with multi-word fallback above 2^100, escape 2^2000, memo on [-10^6, 10^6], Brent, cap 10^5 odd steps plus 10^6 under Brent; cross-checked with Python for m = 31 at modulus 16), `zany.c`, `zverify.py`. Status: method description. Source: D2 7; IC 4.1; M3 9; ST 12. Papers: P1 9; P2 7.1.

V4. Lean development: files Basic, Props, Soundness, RuleH, HSound, Dynamics, Expr, Variants, Window, TwoMinusI; core Lean 4, toolchain v4.35.0, no Mathlib, no `sorry`, `admit`, `native_decide`, `bv_decide`; every `decide` checked by the kernel; `CheckAxioms.lean` prints the axioms of 66 main theorems, at most `propext`, `Classical.choice`, `Quot.sound` (`pi_pow_four`, `digitH_unit_table` with none); clean build about 12 s. Covers Z[i] only. Not formalized: the window step at modulus 1+i, W1, U1 and the thresholds, G19, G40, D2, L3, M9, M10, the census. Status: as stated. Source: LEAN README; TH 8; D2 8. Papers: P1 9; P2 7.2.

V5. Trusted base for the computer-assisted results: the runs establishing the hypotheses of `E2_sound` and `H_sound`; that `gcz` found every cycle reached below 10^7 for the 5+2i map; the C programs, their 128-bit arithmetic and the compiler; agreement of `alpha5p2i_k4.rule` with `digitH`; for M12 also the double-precision evaluation of M9 with margin ≥ 1.4. Status: as stated. Source: D2 7, 8; FAM 5. Papers: P2 7.3.

V6. Not reproducible from the repository as it stands: `lupei.py` (Z30), `gczbig` (M13), the `convrun.py` outputs (M22), the Matthews-Watts products and the Z[i] check of W4 (scratchpad scripts recorded in MW), the outputs behind G10 (`entry2mi.py` exists, no log) and R12 (`mirror29.py` exists, no log). Status: as stated. Source: P1 9; P2 10; INV 5.4. Papers: P1 9; P2 10.

V7. Bounds reached, by map: 5+2i map 3·10^9 (H8); the five (1+i)^2 maps 10^7 (G6); the 3i map and the other (1+i)^3 representatives 10^7 (G16, G17), all 1204 tables 10^6 (G14); -5-2i with D = U ∪ (5-2i)U 10^7 (G29); 1+4i and 4+i with a non-commuting tie-break 10^6 (G35); (1+i)^7 single-cycle maps 10^9 (G45); (1+i)^6 single-cycle maps 10^6 (G41); G_3, G_{3+2i} cycle lists 10^9 (G31, G32); M12 maps 10^9; integer D_k Collatz maps 10^7 (Z14); 7x ± 1 10^15 (Barina), 10^7 (ours); 5+2ω, 5+3ω 10^6 (E4). Throughput about 1.2·10^6 starts per second per core for the 5+2i map. Status: summary of computed bounds. Source: the entries named. Papers: scattered.

## 16. Open questions

Q1. The 5+2i problem (H7). A second cycle with n odd steps and exponent V satisfies Π(α + d_j/z_j) = ε(1+i)^V, which for large z_j forces 2^V ≈ 29^n and arg(5+2i)^n ≈ Vπ/4 mod π/2; the angle condition has no analogue for 3x+1; Steiner and Simons-de Weger bounds might be adapted to both conditions with |ε(1+i)^V - α^n|. Source: D1 13 item 1; D2 9 item 1. Papers: P2 8 item 1.

Q2. Is any map in the window a Collatz map? Every positive result is a finite check: 7x ± 1, the 251 integer maps, the five (1+i)^2 maps, the 305 at (1+i)^3, the 5+2i map and the 30 other commuting single-cycle maps at (1+i)^6, (1+i)^7, 5+2ω. Do single-cycle maps stay a minority at (1+i)^8 (window (256, 512))? Source: D2 9 items 2, 3. Papers: P1 7 item 1; P2 8 item 2.

Q3. Finiteness of cycles for the M12 maps and α = 11 at (1+i)^6: needs an effective lower bound for |επ^V - α^n|/|π|^V, a linear form in two logarithms (real α) or a logarithm and an angle; with a Baker-type bound M9 would make each check finite. Source: FAM 5; D2 9 item 3. Papers: P1 7 item 6; P2 8 item 3.

Q4. Does α = 3 admit a Collatz map at modulus (1+i)^3 (G21)? Source: M3 7, 8. Papers: P1 7 item 2; P2 8 item 5.

Q5. Is there a Collatz map on Z with multiplier -3 at modulus 2 (Z4)? Source: ST 6. Papers: P1 7 item 3.

Q6. Prove divergence for α = 5, 9 or 11 in Z[i], or 5, 9 in Z[ω], by an exact description of orbits on the invariant lines in place of the equidistribution heuristic. Source: D1 13 item 2; D2 9 item 4. Papers: P1 7 item 4; P2 8 item 4.

Q7. A Terras-Everett density theorem in π-adic form (the parity pattern of the first n steps depends only on z mod a power of π), expected to give finite stopping time on a set of density one for N(α) < 2^(k+1) (q^(k+1/(q-1)) in general); it would not rule out divergence (Kionke). Whether the 5+2i map or an iterate meets the Siegel-O'Dwyer hypotheses; a naive encoding mod (1+i)^4 (p = 16) appears to fail (p-1)^2 ρ^d < 1 by a wide margin (rough estimate). Source: D1 13 item 3; D2 9 item 5; LIT 2.9. Papers: P1 7 item 5; P2 8 item 6.

Q8. The 2-adic extension: the 5+2i map extends to Z_2[i]; for 3x+1 the extension preserves Haar measure and is conjugate to the shift (Lagarias 1985, Bernstein 1994); open here. Source: D2 9 item 7. Papers: P2 8 item 7.

Q9. Push the 5+2i check to 10^11 (about nine core-hours). Source: D2 9 item 8; COL 4(b). Papers: P2 8 item 8.

Q10. A map with class-dependent multipliers or moduli (Matthews type) restricting to 3x+1 on the real line of Z[i] with a single cycle; it would need modulus at most (1+i)^2 on the class of 3 mod 4; untested; a single-cycle example would contain the Collatz conjecture. Source: D1 7, 13 item 6; D2 9 item 6; LAG 3 item 2. Papers: P1 7 item 8; P2 9.

Q11. Test the predictions O1 to O4 and the Eisenstein maps dividing by 1-ω. Source: P1 5.2. Papers: P1 7 item 7.

Q12. Has the score 1/M - 1/N, or an equivalent, been used before? Not searched. Source: SC 3. Papers: P1 7 item 9.

Q13. Which least-positive-digit maps with (4/3)2^k < m < (5/3)2^k are Collatz maps on Z (Z28)? Source: IC 6; P1 8 item 3. Papers: P1 8 item 3.

Q14. Literature gaps to close: a direct OEIS search for 5x ± 1 and its cycle (Z11) and a reading of A305057; Lagarias 1990 (possible prior form of E); Matthews and Watts 1984; Heppner 1978 itself; Möller 1978 (presumably the same threshold); Hasse's notes; Belaga-Mignotte; Wirsching; Venturini; Kucinski's claims on divergent orbits; RCWA manual chapter 2 (`GuessedDivergence`); Guy's problem E16; the REU paper. Source: IAP 1; LEI 5; MW 6; SLIT 4; LIT 3.5. Papers: P1 references marked unverified.

## 17. Superseded results

X1. The integer escape threshold 2^100 suggested divergence near the tops of the windows (10^4 or more starts for m = 61, 63 at modulus 32); exact iteration removed it. Replaced by Z19. Source: IC 4.2.

X2. Divergence explained as "λ = α/(1+i)^k real up to a unit" (5 at (1+i)^4 with λ = -5/4; 9, 11 at (1+i)^6; "when λ is not real the direction keeps turning"; 3 and 7 have λ^2 real and converge, "so the condition is on λ itself"); D1's abstract claim "in each divergent case α/(1+i)^k is real up to a unit". Replaced by the invariance of L for all real α and the sign of the drift along L (L3, L5). Source: D1 Abstract, 8, 11; COL 2, 3.7, 4, 5 item 8; FAM 6. P2 6.4 records the replacement.

X3. α = 5 lines: "on them the map acts like a one-dimensional 5x + c map ... drift ln 5 - 2 ln 2 > 0" and "exactly four divisions per odd step"; also "876 of 1000 sample points of the diagonal lines map back". Replaced by the chain drift +0.122 bits with mean v 4.4 and by L4. Source: COL 3.7; D1 8; FAM 6.

X4. Many-cycle maps "are those with λ close to a real multiple of a unit" (13, 15 on the diagonal; 11 ± 10i with λ = (1+21i)/16 nearly imaginary), and "many cycles of the same length suggests families of cycles rather than a finite list". Replaced by near-coincidences of α^n with επ^V (M2, M3) and the complete lists (M12). Source: COL 5 item 8; FAM 1, 3.

X5. The excursion model: θ = 0.142 with n = 4·10^7 (one chunk), "about 123 bits", "122.9 including the 4.86-bit pre-division peak", "the model is accurate". Replaced by H13 (θ = 0.165, the shared point of norm 2^151.2). Source: COL 2, 3.6; D1 9 note.

X6. Cycle counts: α = 5 at (1+i)^4 "36+" (not reproduced with cap 50000), then 34, then at least 35 after the 40-cycle print limit was fixed; α = 11 at (1+i)^6 "23 or 24 cycles" and "the same counts at 10^5 and 10^6"; (1+i)^7 counts 72 for 15 and 545 for 11 ± 10i at 10^6 (now 74 and 766; growth 10^5 to 10^6 was 141 to 166, 64 to 72, 149 to 545). Replaced by G38, G41, G46, M12. Source: COL 3.3, 5 items 3, 7, 8; D1 5, 5.1; FAM 7 item 1. P2 5.1 records the corrections.

X7. Cycle representatives: the 70-cycle of 5-2i "through 16+15i" (COL 3.3) against 15+16i (D1, D2, P2; first-quadrant representative, to be checked as canonical); the 130-cycle of 6+5i "through 199+164i" (COL 3.3), while the rerun shows 164+199i and 199+164i lies on the 6-5i cycle. Replaced by G38, G39. Source: COL 3.3.

X8. Start counts: the gstop runs for G_3 and G_{3+2i} over (10^6, 10^9] had 392,306,389 starts each (3.92·10^8), not 3.93·10^8 (D2 7, COL 3.6); the 5+2i check had 1,174,170,307 starts (1.17·10^9, the sum of the 30 chunks in LOG alpha5p2i_k4.stop), not 1.18·10^9 (COL 3.6 table, D1, D2). Source: LOG G3_k3.stop, G3p2i_k3.stop, alpha5p2i_k4.stop. P2 5.1 records the corrections.

X9. D1 census table entries: (1+i)^7 "4 (8)" single-cycle (should be 8 (4)); "2 to 545; none divergent"; "checked to 10^6"; columns counted up to units only for moduli 6 and 7. Replaced by S2. Source: D1 5.1.

X10. Framing of the 5+2i map: "the first with a single cycle appears at k = 4 ... among maps with the smallest added terms" (D1 abstract); "what singles out H is the smallest level at which a single-cycle rule exists, together with uniqueness at that level" (D1 5.1; COL 4(c)); "H is unique there up to conjugation" with "level", H defined among minimal digit sets only. Replaced by "first in the order by norm" (D2 5.1) and then by G26, which needs no minimality. Source: D1 Abstract, 4, 5.1; COL 4; D2 5.1.

X11. Minimality as the order on digit sets (D2 3.5; "a digit set is minimal if each chosen digit has least norm", D1 2, applied to commuting sets only; ST extended it to all tie-breaks). Replaced as the ordering principle by the score (W15); smallest digits remain a parameter of the census. Source: D1 2; D2 3.3, 3.5; ST 2.

X12. ST's proof of Theorem 4 relied on the census for D = U at (1+i)^3; replaced by the displayed multiplications checked in Lean (G26). Source: ST Thm 4; TH 5, 7.

X13. "Integer rules of this kind may already appear in the literature ... which we have not searched" (D2 3.6); replaced by Barina, Matthews's L_d, Collatz's and Conway's permutations (Z9, Z29). Our 7x ± 1 check to 10^7 is superseded as a result by Barina's 10^15 (Z10). Source: D2 3.6; SLIT 6.

X14. Window prior art: "the encoded map is not of relatively prime type, so Conjecture 3.1 does not cover it, and the criterion that applies is Leigh's Markov-chain Conjecture 7.1 ... [my reading]" (IAP 3.1). Replaced by MW 1985 condition C with β = 1, Conjectures 1 and 2 (W5); Leigh's criterion is his Conjecture 1, Corollary (iii), (4), one-sided. Also D2's attribution to Matthews and Watts 1984 of "almost all trajectories cycle if |m_0 ... m_(d-1)| < d^d" misstates the convergent half (all trajectories eventually periodic). Source: IAP 3.1; LEI 3.1; MW 4.1, 7 items 2, 7.

X15. Leigh's Acta Arithmetica paper dated 1985 (Lagarias, Matthews); the paper carries 1986 and a 1984 receipt date. Source: LEI 1.

X16. COL 1 and D1 said Kucinski's preprint was "not found online" (it was found); Joseph's page range given as 309-317 (it is 309-316); COL attributed arXiv:1907.13302 and 1910.11798 to Matthews (both by Tremblay, on Z). Source: LIT 2.1, 2.2, 2.13.

X17. COL k = 1 remark "N < 4: units only, so every non-trivial single-rule map diverges on average", same content as G1 in old wording. Source: COL 2.

X18. D2's Lean table predates TH and lacks the Dynamics, Expr, Variants, Window and TwoMinusI files; D2 8 says Propositions 1(1), 3 and 5 and the census are not formalized. Replaced by V4. Proposition numbering changed between drafts (D1 Prop 1 to 5 = D2 Prop 2, 3, 4, 5, 1; Lean names `prop1`, `prop3`, `prop5_2` follow D1). Source: D2 8; INV 4.7.1, 4.7.2.

X19. Retired vocabulary: "level k" for modulus (1+i)^k; "expression" and "rule" for map and Collatz map (Lean keeps `Expr`, `Expr.IsRule`); "symmetric", "unit-equivariant", "commutes with units" for commutes with the units; "Collatz-like" for maps inside the window (now "in the window"). Source: INV 1.3.6, 1.3.8, 1.3.9, 1.3.15.

X20. `collatz/papers/collatz-inventory.md` itself, mapped to an old outline, is superseded by this file. The figures it computed for itself (G10, R12, G36 per-multiplier counts and R9's 32 against 16) are carried here with INV as their only written source.
