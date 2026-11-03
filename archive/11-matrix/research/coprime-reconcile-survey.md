# Packed-coprime: reconciling two operands' coprime bases (survey, not a decision)

Oct 30, 2026. Research only. No project code changed, nothing committed. Read: v0.3 §16.6–16.8, `lib/src/mh3.c` (`cbase`, `refine`, `reconcile`, `split_in`, `rebuild`), m3-cost-breakdown, m3-optimization-survey, v0.4 §17.9 (the draw), RULES.md. Simulations are in Python, untimed, and listed under "Evidence".

## 1. What changes when only denominators are factored

The next version's decisions (only denominators are factored, the base is kept in the list as flagged entries, exponents folded into entries) change the problem from m3's:

1. **D_X is the base B_X itself.** In m3, the base (q) also held numerator pieces, so `reconcile` first built (r), the denominator-factor lists D_L and D_R, by scanning every cell. Now every piece is a denominator factor, so D_X = B_X and can be read straight from the flagged entries. **(r) and its cell scan go away.** Reconciliation becomes "make B_L and B_R compatible": every pair is equal or coprime.
2. **Bases are tiny.** Pieces are pairwise coprime 7-smooth numbers above 1, so each piece owns a different, non-empty set of the primes {2, 3, 5, 7}. So **|B| ≤ 4**, and the common refinement of two bases also has at most 4 pieces. At most 16 gcds of numbers no larger than 32767 decide a whole reconciliation. **The gcd count is not the cost. The cost is rewriting entries after a split** (item 4).
3. **Where it is needed.**
   - add: the merge for gcd(b, d) compares B_L's pieces with B_R's.
   - mul: the cross gcds gcd(a, d) and gcd(c, b) have one whole, unfactored side, so they need no reconciliation. But the inner sums s + p merge temporary lists built from both A's and B's pieces, so mul needs B_A compatible with B_B too.
4. **The cost of a split.** Splitting piece q into pieces P_i rewrites, in each operand that holds q, every denominator entry whose exponents include q: one pass over that operand's factored entries. If exponents are positional (slot i ↔ base entry i), appending a piece may also change every entry's layout. m3 rebuilt (f) once per split per holder (`split_in` → `rebuild`).
5. **How often mismatches happen** (simulated, §5). Bases converge to the primes once a matrix has more than a few dozen distinct denominators. **Mismatches come only from small pools** (s = n²/ρ ≤ 64):

   | Job (n, ρ, pool s) | Bases equal | Bases incompatible | Gcds per step, all pairs | Gcds per step, match-first (C3) | Mismatches fixed by primitive roots (C8) |
   | --- | --- | --- | --- | --- | --- |
   | 16, 64, s = 4 | 2% | 64% | 4.7 | 2.4 | 46% |
   | 16, 16, s = 16 | 64% | 20% | 15.1 | 0.24 | 98% |
   | 32, 64, s = 16 | 56% | 20% | 14.9 | 0.28 | 100% |
   | n ≥ 16, s ≥ 64 | about 100% | 0% at s ≥ 256; a few % at s = 64 | ≤ 16 | about 0 | — |

   Typical mismatches are 49 against 7, 75 against {3, 5}, and 21 against {3, 7}. **Where mismatches are frequent, the pools are small, so W is small and each rewrite is short.** The cost limits itself. Each matrix's base only gets finer, and a matrix's denominators are 7-smooth, so a matrix sees at most a handful of splits over its lifetime: about 1.0–1.4 pieces split per mismatching step.

## 2. Candidates

Notation: k_L = |B_L|, k_R = |B_R|; U_L and U_R are the pieces with no equal partner on the other side; E_X is X's number of factored entries.

### C0. m3's in-place refine (v0.3 §16.7), adapted (the baseline)
- **What:** for each r in D_R, scan B_L: gcd(q, r); on a meeting, take S = coprime base of {q, r} and split q and r in every operand that holds them. Restart the scan. Repeat whole passes until a pass splits nothing.
- **Cost:** k_L·k_R + k_R·k_L gcds per pass. Any split costs at least one more full pass to confirm, plus a rescan of X after each split. **One rebuild of each holder's entries per split.** Here that is at most about 32 gcds, plus the rewrites.
- **Structure:** none new. (r) is no longer needed (item 1). The operands are mutated.
- **Combines with:** everything below. Correct as is (§16.7's argument).
- **Fit:** correct but wasteful: repeated passes and one rewrite per split. Its loop exists because m3's D could change under splits of numerator pieces, which no longer happens.

### C1. Bach–Driscoll–Shallit naive factor refinement of the union
- **What:** run the textbook refinement on B_L ∪ B_R in scratch: replace any two non-coprime m₁, m₂ by gcd, m₁/gcd, m₂/gcd until pairwise coprime. This is m3's `cbase`. Then express each operand's pieces over the result S and rewrite each operand once.
- **Cost:** O((k_L + k_R)²) gcds per round, with several rounds possible; O(size³) bit operations in the worst case (BDS). It ignores that each side is already coprime.
- **Structure:** S is scratch, at most 4 entries here. In general its size is bounded only by the step's distinct primes, so it is a **local** of the reconcile function.
- **Combines with:** C3, C4, C5.
- **Fit:** fine at k ≤ 4, but C2 does strictly less work.

### C2. Augment: merge two coprime sets (BDS "augment"; Bernstein's coprime-set merge)
- **What:** both inputs are already coprime bases, so push each r ∈ B_R into a copy of B_L by **09's absorb scan**: gcd with each current piece, divide on h = q, split on 1 < h < q, append any remainder. Each element of one set only has to be pushed against the other set; pairs within a side never need a gcd. When the merge ends, S = cb(B_L ∪ B_R). Then **one rewrite per operand** (C4) maps each old piece to its pieces over S.
- **Cost:** about k_L·k_R gcds, plus a few gcds per split (`cbase` on two numbers); O(size²) bit operations (BDS augment). A single pass, with no repeat-until-stable loop.
- **Structure:** S is the same local as in C1. With only "data like a few integers" here, it is not a structure in this workload.
- **Combines with:** C3 (do it first), C4, C5. **It reuses the build's pass-1 absorb routine**: one solution shared by building and reconciling (RULES, design 3).
- **Fit:** the textbook choice for two coprime sets. Small and exact.

### C3. Match equal pieces first, gcd only the unmatched (a special case of C0–C2)
- **What:** compare the two bases by value (sort-merge if the flagged entries are kept sorted; otherwise k_L·k_R integer compares). **Equal pieces drop out of both sides with no gcd.** *Proof:* if q ∈ B_L equals r ∈ B_R, then q is coprime to every other piece of B_R, because r is. Run gcds only on U_L × U_R. If both are empty (equal bases, the common case at large pools), reconciliation is a comparison only.
- **Cost:** |U_L|·|U_R| gcds: measured 0.24–2.4 per step, against 4.7–15 for all pairs.
- **Structure:** none.
- **Combines with:** C0, C1 or C2. Reordering m3's `h = gcd(q, r); if h = 1 or q = r` to test q = r first is the smallest form of it.
- **Fit:** large saving at almost no code. Name it as a special case (RULES, design 2): "S-eq: equal pieces skip the gcd; equal bases skip reconciliation".

### C4. One batched rewrite per operand (how splits are applied)
- **What:** after S is final (C1 or C2), make one pass over each operand that holds a split piece. Each entry's exponents over the old pieces are replaced by exponents over S: q = ∏ P_i^{e_i} gives ν·e_i. Each entry is re-sorted or re-laid out once. The flagged base entries are replaced in the same pass.
- **Cost:** one pass over E_X per changed operand per reconciliation, instead of one per split (m3). Exponent products ν·e_i are overflow-checked: overflow is an error.
- **Structure:** none if the rewrite is in place. If the new layout is wider, it is a reallocation, the same structure (as v0.3's (f′)). The old-piece → S map has at most 4 × 4 small exponents here; in general it shares the reconcile function's one local with S, as one array of records.
- **Combines with:** C1 or C2.
- **Fit:** the main cost lever when mismatches are frequent, and those are exactly the small-W jobs.

### C5. Seed the result's base with S
- **What:** the result's denominators are products of divisors of b and d over S, except for leftovers (h_q = gcd(t, q^ν) not a power of q). So start the result's base from S. Its pack-time factoring then finds most entries by divisibility tests against S, and gcd-splits only on a leftover.
- **Cost:** saves the result's pass-1 gcds (m3's dominant cost: 84–99% of Pack-coprime's operations). It does not change reconciliation's own cost.
- **Structure:** none. S becomes the result list's flagged base entries, part of its one stored structure. Pieces that divide no result denominator are dropped at the pack.
- **Combines with:** C2 + C4.
- **Fit:** good. It also makes the result's base compatible with both operands from birth, so a later step that combines the result with an operand (sop's R = P + Q, pos) starts with C3's equal-bases case.

### C6. Lazy, per cell: refine only what meets, when it meets
- **What:** in the step, find the incompatible pairs (U_L × U_R gcds, as C3). Don't rewrite. Per cell, when F(b) and F(d) touch an incompatible group of pieces, get that group's gcd by Euclid on the group's part (as §16.6's per-piece gcd). Elsewhere, merge.
- **Cost:** the same gcds per step as C3, plus one Euclid per cell that touches a bad group: up to n² (add) or n³ (mul) extra gcds, against C4's one O(W) rewrite. **mul's temporaries s, p would mix incompatible pieces**, so their lists cannot be merged by value. Each sum would need the fallback, or a split of the live temporary lists (v0.3 §16.8 row 3).
- **Structure:** one incompatibility flag per base entry (a field, not a structure), or a small mask.
- **Combines with:** C3.
- **Fit:** poor. It moves an O(W) one-off into per-cell work and complicates every gcd case. m3-optimization-survey already noted "lazy" adds deferred-refinement state.

### C7. One shared base across all matrices
- **What:** a single global coprime base, with matrices referring to its pieces. A split must reach every live matrix: either scan them all (needs a registry), or keep a split history (old piece → children with exponents), so old factorizations stay valid and are normalised lazily.
- **Cost:** after warm-up, reconciliation is identity comparison with zero gcds. Here the global base reaches {2, 3, 5, 7} after the first few matrices. A split still costs rewrites, or history walks of O(depth) per read.
- **Structure:** **yes, a new kind.** It is owned by no single object, so it is none of RULES's stored, temporary or local. Its history or registry would be a second one. It needs the author's discussion. It also breaks "the base is per matrix, as stores are" (v0.3 §16.7) and makes results depend on the order of operations across jobs.
- **Combines with:** C3.
- **Fit:** it would be the cheapest in steady state on this workload, but it is outside the rules as written.

### C8. Canonical pieces: perfect-power-free (primitive roots)
- **What:** store each new piece as r, the integer with q = r^k and r not a perfect power, adding k to the exponents. Then 4 and 8, or 49 and 7, are the same piece in every matrix. This is the natural refinement used in perfect-power detection (Bernstein–Lenstra–Pila).
- **Cost:** a perfect-power test per new piece, once per piece per matrix lifetime: integer k-th roots for prime k ≤ 63, by schoolbook root and overflow-checked powering. It removes 46–100% of the measured mismatches; the rest are composite-support cases like 75 against {3, 5}.
- **Structure:** none.
- **Combines with:** C2–C5. It is general, not workload-specific, but name it as a special case: "S-pow: prime-power and perfect-power pieces".
- **Fit:** good on the 49/7 class, which dominates once s ≥ 16.

### C9. Canonical pieces: split off small primes (a named special case, B = 7 here)
- **What:** whenever a piece is made, divide it by each prime p ≤ B (2, 3, 5, 7: four constants, not a table). Each p found becomes its own piece, and only the B-rough remainder goes through gcd refinement. All matrices then share the same small-prime pieces, which are always equal or coprime by value.
- **Cost:** π(B) divisibility tests per new piece. **On a 7-smooth workload, every base is a subset of {2, 3, 5, 7}: zero splits, zero rewrites and zero leftover splits**, and reconciliation is C3's comparison only. It is general in form: rough pieces still go through C2.
- **Structure:** none.
- **Combines with:** C2 + C3 + C4 + C5 for the general part.
- **Fit:** it ends reconciliation on this workload. **Risk:** it is tuned to the draw and moves the model towards Packed-235 (v0.5), so the measurement would then test little of the coprime machinery. It must be its own row (RULES, design 2), after the general version is measured.

### C10. Canonical pieces: full primes when cheap
- **What:** if a piece is below a bound (e.g. 2³²), trial-divide it to √q and store primes. Larger pieces stay composite. A piece known to be prime, with a flag bit on its base entry, skips gcds against other prime-flagged pieces.
- **Cost:** up to about √q / 2 candidates per new piece: up to 90 for q ≤ 32767, but 2¹⁵ for q near 2³². This is Pack-primes' trial division, the 64–158× Ir loss of m3, though here it runs per piece, not per number.
- **Structure:** none (a flag bit).
- **Combines with:** C3, C9.
- **Fit:** on this draw C9 gives the same result much more cheaply. Only worth it for workloads with larger primes.

### Not suitable: Bernstein's essentially-linear-time coprime base
- **What:** product and remainder trees ("factoring into coprimes in essentially linear time").
- **Why not:** it needs multi-precision products, since piece products exceed 64 bits, and fast multiplication to pay off. That breaks the 64-bit, self-contained and schoolbook-first rules, and it gains nothing at k ≤ 4.

## 3. Correctness notes (any candidate)

- **Values, cells and the numerators' entries never change.** Only flagged base entries and denominator exponents do, so matching the control value for value is unaffected.
- **After C2, S is a coprime base of B_L ∪ B_R**, so every pair a merge of the step compares is equal or coprime. Keep m3's audit (`SK_AUDIT` in `reconcile`) and the unit tests: {12, 18}, {15, 45}, {15, 225}, {36, 6, 4}, plus 49/7 and 75/{3, 5}.
- **Leftover splits inside mul's sums** (v0.3 §16.8 row 3) still occur with composite pieces. They cannot occur when every touched piece is prime: under C9 on this draw they never occur.
- **Mutating operands is safe and amortises:** a base only gets finer, so a later step on the same input meets fewer splits. Inputs are rebuilt every run, as in m3.
- **Overflow:** a rewritten exponent ν·e_i, and a piece's power q^ν formed for a per-piece gcd, are overflow-checked. Overflow is an error.

## 4. Recommendation

- **General build:** C2 (absorb one base into a copy of the other, sharing the build's pass-1 routine) with C3 (equal pieces first) and C4 (one rewrite per operand). Then C5 (seed the result's base with S). No new structure, and S is a few integers here (in general, the reconcile function's one local).
- **Then measure, and add as named special-case rows:** C8 (primitive-root pieces) as the general canonicaliser, and C9 (split off 2, 3, 5, 7) as the workload's special case, which removes reconciliation entirely. **Not recommended:** C6 (lazy per cell), and C7 (shared base) unless the author opens a new structure kind.

## 5. Evidence (simulations; scratchpad, not in the repo)

- **Draw:** v0.4 §17.9's steps (primes 2, 3, 5, 7, each with probability 1/p, repeats with probability 1/p, reject above 32767, random sign on the numerator), with pools of s = n²/ρ distinct reduced rationals and cells filled by choice from the pool.
- **Not replicated:** the exact order of random calls, so the figures are statistical, not job for job.
- **Method:** bases computed with naive refinement over each matrix's distinct denominators; 60–400 trials per point.
- **Unpooled cells (each drawn independently):** incompatible A/B bases in 17–55% of pairs at n ≤ 3, 24% at n = 4, and 0% at n ≥ 8.
- **Sums and products:** at n = 16, ρ = 64, A + B's base is incompatible with an operand's in 65% of trials, and A·B's with A + B's in 43%. Both are 0% at s ≥ 64.

## Sources

- E. Bach, J. Driscoll, J. Shallit, "Factor refinement", J. Algorithms 15 (1993); tech. report TR883 (1989): https://minds.wisc.edu/items/dc484038-b334-446c-aece-27209f8c1497
- Naive refinement O(size³), augment O(size²), merge of two coprime sets: "On the factor refinement principle and its implementation" (slides): https://www.sambuz.com/doc/on-the-factor-refinement-principle-and-its-implementation-presentation-1089885
- D. J. Bernstein, "Factoring into coprimes in essentially linear time", J. Algorithms 54 (2005), 1–30: https://cr.yp.to/coprimes.html
- D. J. Bernstein, H. W. Lenstra, J. Pila, "Detecting perfect powers by factoring into coprimes", Math. Comp. 76 (2007): https://www.ams.org/journals/mcom/2007-76-257/S0025-5718-06-01837-0
- J. Buchmann, F. Eisenbrand, "On factor refinement in number fields", Math. Comp. 68 (1999): https://www.ams.org/journals/mcom/1999-68-225/S0025-5718-99-01023-6
