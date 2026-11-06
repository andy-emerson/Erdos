# Lean 4 proofs for `collatz/papers/gaussian-collatz.md`, `theorems.md`, `quotient.md` and `general-theory.md`

Core Lean 4 only (toolchain `leanprover/lean4:v4.35.0`). No Mathlib or other dependencies,
no `sorry`/`admit`, no `native_decide`/`bv_decide`, no new axioms. Tactics used: `omega`,
`decide`, `simp`, `grind` (for polynomial identities over `Int`), `induction`, `rcases`.

## Build

```
cd collatz/research/lean
lake build                       # builds the library, a few seconds to ~20 s
lake env lean CheckAxioms.lean   # prints the axioms of every main theorem
```

Expected axioms: at most `propext`, `Classical.choice`, `Quot.sound`. A clean build of all
files takes about 16 to 30 s. `CheckAxioms.lean` prints 157 theorems (6 November 2026: 122 with
all three axioms, 20 with `propext` and `Quot.sound`, 5 with `propext` only, 10 with none; no
`sorryAx`).
Build output goes to `.lake/` (ignored by `.gitignore`).

## Files

- `GaussianCollatz/Basic.lean`: Gaussian integers `GInt` (structure of two `Int`), `+`, `-`, `*`,
  norm `N z = a² + b²` (a `Nat`), units `IsUnit u := u = 1 ∨ u = I ∨ u = -1 ∨ u = -I`, `π = pi = 1+i`,
  divisibility `x ∣ y := ∃ q, y = x * q`, powers `pow`, `Even`/`Odd` (`π ∣ z` or not),
  `pi_dvd_iff : π ∣ z ↔ (re - im) % 2 = 0`, exact division `divPi` with `pi * divPi z = z`,
  and iteration `iter f n z`.
- `GaussianCollatz/Props.lean`:
  - `prop3_int (a b : Int) : a*a + b*b ≠ 3`, `prop3 (z) : N z ≠ 3` (Proposition 3).
  - `prop1`: for `k ≤ 2`, any `α` and any digit function `d` that depends only on the class mod
    `π^k`, sends odd to odd and makes `π^k ∣ α z + d z` for odd `z`, there is an odd `z` with
    `d (-z) ≠ -(d z)` (Proposition 1). Key fact `pi_sq_dvd_two_mul : π² ∣ z - (-z)`.
  - `prop5_2 : π^k ∣ 10 → k ≤ 2`, and `prop5_2'`: if `π^k ∣ 3n+1` for every odd natural `n`
    then `k ≤ 2` (Proposition 5(2)).
- `GaussianCollatz/Soundness.lean`: `drop_below_start_sound`, the soundness of the
  drop-below-start check of Section 12. For any `T` with `T z = z/π` when `π ∣ z` and
  `T (u z) = u (T z)` for units `u`: if every odd `z` with `re > 0`, `im ≥ 0`, `N0 < N z ≤ B`
  has some `n` with `N (T^n z) < N z`, then every `z` with `N z ≤ B` (including `0`, even `z`
  and every associate) has some `n` with `N (T^n z) ≤ N0`.
- `GaussianCollatz/RuleH.lean`: the rule `H` (`α = 5+2i`, `k = 4`). `pi_pow_four : π⁴ = -4`;
  `four_dvd_class` (whether `4 ∣ αz + d` depends only on `z mod 4`); `tableH_reps`
  (the 8 representatives are odd and the listed digit works, by `decide`); `tableH_distinct`
  and `tableH_covers` (the representatives are exactly the 8 odd classes mod 4);
  `digitH_correct` (the digit is correct for every odd `z`); `H_cycle` (from `1`, one odd step
  to `4+4i`, then five divisions by `π` through even values to `-1`, and back from `-1` to `1`).
- `GaussianCollatz/HSound.lean`: the check applied to `H` itself. `H_div : ∀ z, π ∣ z → H z = divPi z`;
  `digitH_unit_table` (by `decide`: on the 8 odd residue pairs, every unit sends the digit of a class
  to the digit of the image class); `digitH_unit` (`d(u z) = u d(z)` for odd `z`); `divPi_unit_mul`;
  `H_unit : ∀ u z, IsUnit u → H (u * z) = u * H z`; and the corollary `H_sound`, which is
  `drop_below_start_sound` with `T := H`: if every odd `z` with `re > 0`, `im ≥ 0`,
  `N0 < N z ≤ B` has `n` with `N (H^n z) < N z`, then every `z` with `N z ≤ B` has `n` with
  `N (H^n z) ≤ N0`.
- `GaussianCollatz/Dynamics.lean` (`theorems.md`, Sections 1 to 4, for any map `f`): `Periodic`,
  `Reaches` (forward orbit; for periodic points, the same cycle), `OnTrivialCycle`, `Trivial`
  (every odd `z` has an iterate on a cycle containing a unit), `Commutes` (`f (u z) = u f z`),
  `conj`, `ord`.
  - Theorem 2.1: `trivial_cycles_le_four` (any map: a list of points on trivial cycles, pairwise on
    different cycles, has length at most 4); `trivial_cycle_has_unit`; under `Commutes`:
    `unit_periodic_iff`, `unit_cycle_eq` (the cycle of a unit `u` is `u C₁`), `stab_iff`
    (`u C₁ = C₁ ↔ u ∈ C₁`), `stab_mul`, and `trivial_cycle_count` (exactly the three cases
    `U ∩ C₁ = {1}`, `{±1}`, `U`, with 4, 2, 1 trivial cycles).
  - Theorem 3.1: `unit_cycles` (if `w = f 1` is a unit: `f u = u w`, `f^n u = u w^n`, units stay
    units, `f^n u = u ↔ w^n = 1`, least period `ord w`), `no_unit_cycle`.
  - Lemma 4.3: `trivial_transfer`; and `not_trivial_of_invariant` (an `f`-invariant set of
    non-units containing an odd element rules out triviality).
- `GaussianCollatz/Expr.lean`: `oddPart`/`val` (odd part and exponent of `π`, by fuel `N w`),
  `structure Expr` (`α` odd, `k`, digit table `d` depending only on the class mod `π^k`, with
  `π^k ∣ α z + d z` for odd `z`), `Expr.T`, `Expr.S` (`0` on even inputs and when
  `α z + d z = 0`), `Expr.Symmetric`, `Expr.IsRule := Trivial E.S`. Lemmas: `N_mul`,
  `pi_dvd_mul` (`π` is prime), `pow_dvd_of_odd_mul` (Lemma 1.2(b)), `pow_odd_unique`,
  `oddPart_spec`, `oddPart_pow_mul`, `Expr.S_spec`, `Expr.S_eq` (one-step certificate),
  `Expr.S_commutes` (Lemma 1.4(c)), `Expr.d_odd`, `Expr.d_inj` (Lemma 1.2(c)), and the
  expression forms `Expr.trivial_cycles_le_four`, `Expr.trivial_cycle_count`, `Expr.unit_cycles`.
- `GaussianCollatz/Variants.lean` (Section 4): `Expr.rotate E u` (`α ↦ uα`, table `z ↦ d (u z)`,
  same digit set) and `Expr.mirror E` (`α ↦ ᾱ`, table `z ↦ conj (d (conj z))`).
  Without symmetry: `rotate_S` (`S_{uα} z = S_α (u z)`), `mirror_S`
  (`S_ᾱ z = (-i)^v conj (S_α z̄)`, `v = vstep (conj z)`), `mirror_norm`. With symmetry:
  `rotate_symmetric`, `rotate_S_mul`, `rotate_iter` (`u^n S_α^n`), `rotate_norm`, `rotate_rule`,
  `rotate_S_one`; `mirror_symmetric`, `mirror_iter` (`(-i)^{V_n(z̄)} conj (S_α^n z̄)`),
  `mirror_norm_iter`, `mirror_rule`.
- `GaussianCollatz/Window.lean` (Section 5): `Expr.fixed_point` (Lemma 5.1),
  `Expr.fixed_point_not_rule`, `Expr.digits_units` (Corollary 5.2), `alpha_k3`, `alpha_k4`
  (Lemma 5.3), `units_mod_pi3`, the certificate lemmas `Expr.no_rule_of_cycle3` and
  `Expr.no_rule_k4`, and Theorem 5.4 `Expr.no_symmetric_rule_below_29`: a symmetric expression
  with `2^k < N α < 2^(k+1)` and `N α < 29` is not a rule, for any digit table. The explicit
  cycles at modulus `π^3` (for `α = 3, 3 ± 2i`) are checked by `decide`.
- `GaussianCollatz/TwoMinusI.lean` (Section 6, Remark 3.2): `drop_below_start_sound_all`
  (soundness of the all-quadrant check of `gstopall.c`, no condition on units), the expression
  `E2` (`α = 2 - i`, modulus `π²`, digit `i` for `a` odd, `b` even, `1` for `a` even, `b` odd),
  `digit2_class`, `digit2_correct`, `E2_T_div`, `E2_cycle` (`S 1 = -i`, `S (-i) = -1`,
  `S (-1) = 1`, `S i = -i`), `E2_I_not_periodic`, `E2_not_symmetric`, `E2_T_ne_zero`, and the
  corollary `E2_sound`: if every odd `z` with `1 < N z ≤ B` has a `T`-iterate of smaller norm
  (the `gstopall` check with `B1 = 1`, `B2 = B`), every nonzero `z` with `N z ≤ B` reaches
  `1`, `-i` or `-1`.
- `GaussianCollatz/Quotient.lean` (`quotient.md`): associates `Assoc`, the setoid and the quotient
  `Cls = ℤ[i]/U` (`cls`, `cls_eq_iff`, `cls_one_iff`: `[z] = [1] ↔ IsUnit z`), `RespectsAssoc`,
  `Commutes.respects`, the induced map `descend` with `descend_cls`, `descent_iff` (a quotient map
  with `F [z] = [f z]` exists iff `f` respects associates), `descend_unique`, `sq_respects` and
  `sq_not_commutes` (strictly weaker), `exists_commuting_same_descend` (every quotient map comes
  from a commuting map, if `f 0 = 0`), `qiter`, `descend_iter`, `pigeon`.
  Transfer (Theorem 3.2): `CollatzOn P f`, `QCollatzOn P F` (every class reaches `[1]`, `[1]`
  periodic), `transfer`, `Expr.rule_iff_quotient`, `transfer_nonzero`. Lifts (Theorems 4.1, 4.2):
  `lift_cycle`, `qperiod_dvd`, `lift_period_dvd`, `commuting_lift`, `commuting_lift_periodic`,
  `commuting_stab` (the cycle of `z` contains `v z` iff `v ∈ ⟨ε⟩`), and the example `ex3`,
  `ex3_respects`, `ex3_cycle` (a lift of length 3). Corollaries 6.1 and 6.2 without symmetry:
  `Expr.rotate_respects`, `Expr.rotate_descend`, `Expr.rotate_rule_of_respects`, `qconj`,
  `Expr.mirror_respects`, `Expr.mirror_descend`, `Expr.mirror_rule_of_respects`. Theorem 5.1:
  `Expr.big_point` (Lemma 5.0'), `Expr.symmetric_of_T_respects`, `Expr.symmetric_of_S_respects`,
  `Expr.respects_iff_symmetric`, `Expr.symmetric_of_twisted`. Integers (`IntCollatz`):
  `C3_not_respects`, `level_one_not_respects`, `B7_commutes`, `B7_quotient`, `pm3_commutes`,
  `tw3_respects`, `tw3_not_commutes`, `tw3_pm3`, `lifts_differ`.
- `CheckAxioms.lean`: `#print axioms` for every main theorem (not part of the library).
- `GaussianCollatz/Abstract.lean` (`general-theory.md`, Sections 7, 8 and 15; no imports): the
  abstract layer, namespace `Abstract`. `FinAction G X`: a finite group (operations, the group
  axioms, a duplicate-free list `elems` of all elements) acting on a type `X`; `Orb`, `Respects`,
  `Equivariant`, `Free`, `gpow`, the orbit space `Q` with `cls`; `iter`, `Periodic`, `Reaches`,
  `OnTrivial A e f x` (periodic, cycle meeting `G e`), `CollatzOn A e P f`, `QCollatzOn`.
  Descent: `descent_iff`, `descend_unique`, `descend_iter`. Pigeonhole: `orb_mem`, `pigeon_list`,
  `pigeon`. Transfer: `transfer_orbit`, `transfer`. Trivial cycles: `trivial_cycles_le` (any map,
  at most `|G|`), `trivial_cycle_eq`, `exists_cycle_reps`, `exists_stab_list`,
  `trivial_cycle_count` (`|L| · |H| = |G|` for equivariant maps, `H` the stabilizer),
  `trivial_cycles_dvd`. Unit cycles: `orbit_iter`, `orbit_iter_period`, `gpow_eq_one`. Lifts:
  `lift_cycle`, `qperiod_dvd`, `lift_period_dvd`, `equivariant_lift`,
  `equivariant_lift_periodic`, `equivariant_stab`, `equivariant_stab_free`. Rotation and
  conjugation: `agree_respects`, `agree_descend`, `agree_collatz`, `equivariant_agree_collatz`
  (the transfer lemma R4), `conj_collatz` (the mirror).
- `GaussianCollatz/AbstractNorm.lean` (`general-theory.md`, Section 9; no imports): Theorem 9.1
  over any commutative ring (`Lean.Grind.CommRing`). `NormData` (multiplicative `N : R → ℕ`,
  `N 1 = 1`, `N 0 = 0`, positive off `0`, `1` on the units, `N π ≥ 2`); `core_T`, `core_S`
  (Lemma 5.0 of `quotient.md`), `commutes_of_T_respects`, `commutes_of_S_respects` (respecting
  associates at large points of every class forces the table to commute; the big-point property
  is a hypothesis).
- `GaussianCollatz/AbstractGInt.lean`: the instances. `U4 : FinAction (Fin 4) GInt` (`i^a` acting
  by multiplication) with the bridges `U4_orb_iff` (orbits are associates), `U4_respects_iff`,
  `U4_equivariant_iff`, `U4_free`, `iter_eq`, `periodic_iff`, `reaches_iff`, `onTrivial_iff`,
  `collatzOn_iff`. Corollaries proved from the abstract layer only, with the statements of the
  earlier theorems (checked by `example : @transfer = @transfer_of_abstract := rfl` and so on):
  `transfer_of_abstract`, `trivial_cycles_le_four_of_abstract`, `trivial_transfer_of_abstract`,
  `lift_cycle_of_abstract`, `commuting_stab_of_abstract`, `Expr.rotate_rule_of_respects'`,
  `Expr.mirror_rule_of_respects'`, `Expr.symmetric_of_T_respects'`,
  `Expr.symmetric_of_S_respects'` (through the instance `instCommRingGInt` and `gaussNorm`),
  and `unit_iter_of_abstract` (`f^n u = u w^n`). New: `trivial_cycles_dvd_four` (the number of
  trivial cycles of a commuting map divides 4). Integers: `IntCollatz.Z2 : FinAction (Fin 2) Int`,
  `B7_equivariant`, and `barina_iff` (every positive orbit of `7x ± 1` reaches 1 iff `7x ± 1` is
  a Collatz map on `ℤ \ {0}`).
