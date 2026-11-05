# Lean 4 proofs for `papers/gaussian-collatz.md`

Core Lean 4 only (toolchain `leanprover/lean4:v4.35.0`). No Mathlib or other dependencies,
no `sorry`/`admit`, no `native_decide`/`bv_decide`, no new axioms. Tactics used: `omega`,
`decide`, `simp`, `grind` (for polynomial identities over `Int`), `induction`, `rcases`.

## Build

```
cd research/collatz/lean
lake build                       # builds the library, a few seconds to ~20 s
lake env lean CheckAxioms.lean   # prints the axioms of every main theorem
```

Expected axioms: at most `propext`, `Classical.choice`, `Quot.sound`.
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
- `CheckAxioms.lean`: `#print axioms` for every main theorem (not part of the library).
