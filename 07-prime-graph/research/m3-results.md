# M3: the M1 library against GMP and FLINT

Oct 28, 2026 · Andrew Emerson

This compares the accepted M1 library (64-bit) with GMP `mpq_t`, FLINT `fmpq`, and FLINT `fmpz` numerators over one shared denominator. The rule was fixed before measurement ([m3-rule.md](m3-rule.md), revised after the pre-run review and before any measurement):
- 12 interleaved rounds;
- 95% bootstrap intervals;
- a null control;
- every contender given its best normal form;
- every result checked against Python's `fractions`.

The full tables are in [`test/m3/results/m3.md`](../test/m3/results/m3.md). The run took 46 minutes.

## Answers

Over 46 jobs, against the best library:

| Verdict | Jobs |
| --- | --- |
| **M1 faster** (9) | decimal and money columns (sum 1.9×, a·b + c 2.5–2.9×), the decimal chain (9.2×), mixed-denominator sums (1.3–1.9×), mixed chains (4.6–6.3×) |
| **tie** (3) | mixed-denominator a·b + c (two jobs), 100 distinct denominators |
| **M1 slower** (34) | single values at every size (0.14–0.54), unrelated denominators (0.72), 10,000 distinct denominators (0.58), two-large-prime denominators (0.16–0.27), and every point of both reuse sweeps |

Against the per-value libraries alone, M1 is 8–37× faster than GMP and FLINT rationals on the one-scale and chain workloads.

**Pay-back of the entry cost: none within the range tested,** on either axis (up to 100 uses per value, or 200 uses per distinct denominator).

## What the numbers show

1. **Where values share a scale, M1 wins,** even against FLINT doing the same common-denominator technique by hand: 1.3–9.2× across sums, elementwise operations and chains. This is the array form with lazy reduction working as intended.
2. **On single values, M1 is slower than FLINT even once entry is excluded.** In the reuse sweep at 100 uses per value, M1's entry is only 2% of its time, yet its operations take 36 ms against FLINT's 22 ms. So the gap is in the operations themselves, not only in entry.
3. **The likely reason is the size.** Within 64 bits, a gcd is cheap (a short loop on machine words), so the gcd that the idea removes is a small cost. M1's graph adds memory indirection on every operation: handles, edge lists, factor lookups. At this size the bookkeeping costs more than the gcd it saves. The concept paper's argument is about cost as the numbers grow, where the gcd dominates. The 64-bit slice is exactly where the gcd matters least.
4. **Entry with large prime factors is prohibitive.** For denominators that are products of two large primes, M1's entry, dominated by Pollard rho, took up to 3.5 s where FLINT took 5 ms, and was 91–100% of M1's time. Even at 200 uses per denominator, M1 ran at 0.11 of FLINT. Factoring at entry is the wrong tool here; gcd-based refinement, as in concept paper v0.2, avoids factoring altogether.
5. **Many distinct denominators** (10,000) ran at 0.58 of FLINT.

## Conclusions

- **The array form is a genuine win** over the best existing technique, on one-scale and mixed data.
- **On single values and the "middle ground" between values, the 64-bit M1 does not yet show the benefit the paper predicts.** Two causes are identifiable:
  - **graph bookkeeping that outweighs cheap 64-bit gcds;**
  - **factoring at entry.**

  Both point to the same next steps. Entry should use gcd-based refinement, not rho. And the single-value claim should be tested where gcds are expensive, at larger sizes, which is the arbitrary-precision milestone. The single-value path's per-operation overhead also needs profiling.
- **Scope:** all values fit 64 bits; GMP and FLINT are general. Single-threaded throughout.
