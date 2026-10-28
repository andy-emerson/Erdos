# M3 peer comparison: pre-registered decision rule

Oct 28, 2026 08:25 CDT · written by `test/m3/m3_bench.py --write-rule` before any measurement of M3.  The measured run refuses to start unless this file contains the rule below verbatim.

**Decision rule for M3 (fixed before any measurement: in research/m3-rule.md, and in the header of results/m3.md; the
harness refuses a measured run unless research/m3-rule.md holds this text).**

1. **Rounds.**  Each job runs 12 interleaved rounds.  In every round each row of the job (M1, M1's null copy, and
   each library contender) runs once, as its own process, in an order rotated by one row per round, so the rows of a
   round are paired.  A row's time in a round is its **total**: entry (conversion from the raw int64 inputs; for M1
   including creating its graph) + operation + exit (every result read out in lowest terms as two int64), the
   minimum over the invocation's warm reps of one rep's sum.  Entry is reported separately and is included in the total.
2. **Checks.**  Every result of every invocation is compared with the exact value computed by Python's `fractions`:
   the numerator and the positive denominator in lowest terms must both be equal.  A row that fails any check in any
   round, or is unable, is **n/a** for that job.
3. **Ratio.**  For a contender C, the ratio is **T(C) / T(M1)**, T being the median of the row's per-round totals; above
   1, M1 is faster.  Its 95% interval [lo, hi] is the 2.5th-97.5th percentile of the same ratio over 10,000 bootstrap
   resamples of the rounds (rounds drawn with replacement, the same draw for both rows, so the pairing is kept; seeded).
4. **Null control.**  M1 against its own second copy on the same job, by 3.  The job's **null spread** is
   max(1 - lo, hi - 1, 0) over that interval; the band half-width is **h = max(5%, null spread)**.
5. **Verdict per job and contender:** **faster** (M1 faster) if lo > 1 + h; **slower** (M1 slower) if hi < 1 - h;
   otherwise **tie** (the interval overlaps the band [1 - h, 1 + h]).  **n/a** if either row is n/a or the null control is.
6. **Best library.**  Per job, the best library is the library row (mpq, fmpq, fz where it runs) that passed every
   check with the lowest T.  The job's verdict is M1's verdict against the best library.
7. **Summary answers.**  (a) **Where is M1 faster than the best library:** the jobs whose verdict is faster.
   (b) **Where is it slower:** the jobs whose verdict is slower.  (Ties and n/a are listed too.)
   (c) **Pay-back of entry:** in each reuse family (R1k, Rpq), the jobs differ only in U, the uses per value (each value
   is an operand of exactly U operations; same number of operations), over the grid U = {1, 3, 10, 30, 100}.  M1's entry cost **pays
   back from** the smallest U in the grid at which the verdict is faster at that U and at every larger U of the grid;
   it reaches **parity from** the smallest such U with tie or faster.  If the verdict at U = 100 is not faster
   (resp. tie or faster), the answer is "not within 100 uses".  An n/a job breaks the run of U's above it.
8. **Calibration.**  Before the rounds, a separate calibration (3 invocations of each form, alternating) picks per job
   the faster FLINT fmpq form of a*b + c (fmpq_mul + fmpq_add, or fmpq_set + fmpq_addmul) by median total.  Its times
   are reported but carry no verdict.  No other comparison carries a verdict.


## Run plan

12 rounds, 2 warm reps per invocation, seed 20261028, 10,000 bootstrap resamples, band floor 5%.  Rows: pg, pg-null, mpq, fmpq, fz (fz only where the inputs have a shared denominator below 2^63).  Workloads:

- **DEC** (decimal columns): three-place decimals in (-10^4, 10^4); n = 1,000,000; jobs sum, fma; rows pg, pg-null, mpq, fmpq, fz
- **CH-dec** (decimal columns): expression chain x <- x*b + c, K = 8: x0, c three-place decimals in (-10, 10), b tenths in (-2, 2); n = 500,000; jobs chain; K = 8; rows pg, pg-null, mpq, fmpq, fz
- **MONEY** (money): two-place amounts in (-10^5, 10^5); n = 1,000,000; jobs sum, fma; rows pg, pg-null, mpq, fmpq, fz
- **MIX12** (mixed small): |num| < 10^6 over denominators 1..12; n = 1,000,000; jobs sum, fma; rows pg, pg-null, mpq, fmpq, fz
- **MIX90** (mixed 90/10): 90% three-place decimals in (-10^4, 10^4) over 1000, 10% |num| < 10^6 over 1..12; n = 1,000,000; jobs sum, fma; rows pg, pg-null, mpq, fmpq, fz
- **U1k** (unrelated): |num| < 10^6 over denominators 1..1000 (no column sum: its common denominator is far beyond 64 bits); n = 200,000; jobs fma; rows pg, pg-null, mpq, fmpq
- **S8** (single values): independent pairs, |num| < 2^8, denominators in [2^7, 2^8); operations = 500,000; jobs add, mul; rows pg, pg-null, mpq, fmpq
- **S16** (single values): independent pairs, |num| < 2^16, denominators in [2^15, 2^16); operations = 500,000; jobs add, mul; rows pg, pg-null, mpq, fmpq
- **S24** (single values): independent pairs, |num| < 2^24, denominators in [2^23, 2^24); operations = 500,000; jobs add, mul; rows pg, pg-null, mpq, fmpq
- **S30** (single values): independent pairs, |num| < 2^30, denominators in [2^29, 2^30); operations = 500,000; jobs add, mul; rows pg, pg-null, mpq, fmpq
- **SH30** (single values): pairs over one shared denominator: |num| < 2^30, denominators from a pool of 1,000 in [2^29, 2^30); operations = 500,000; jobs add, mul; rows pg, pg-null, mpq, fmpq
- **P-sh** (two large primes): a*b + c, a over 2,000 distinct p*q (primes in [2^16, 2^30): 32-60 bits) from a pool of 64 primes (each prime in about 62 denominators), c over a's denominator, b over 1..4; n = 200,000; jobs fma; rows pg, pg-null, mpq, fmpq
- **P-ind** (two large primes): as P-sh, the 2,000 p*q from 4,000 primes in disjoint pairs (no prime shared); n = 200,000; jobs fma; rows pg, pg-null, mpq, fmpq
- **N100** (many denominators): a*b + c, a over 100 distinct denominators (half in [2, 2^31), half in [2^32, 2^40)), c over a's denominator, b over 1..12; n = 200,000; jobs fma; rows pg, pg-null, mpq, fmpq
- **N10k** (many denominators): as N100 with 10,000 distinct denominators; n = 200,000; jobs fma; rows pg, pg-null, mpq, fmpq
- **G-mix4** (growth chains): growth chain x <- x*b + c, K = 4: x0, c |num| < 10^4 over 1..12, b over 1..4 with |b| < 2 (denominators grow up to 4x per step); n = 500,000; jobs chain; K = 4; rows pg, pg-null, mpq, fmpq, fz
- **G-mix8** (growth chains): growth chain x <- x*b + c, K = 8: x0, c |num| < 10^4 over 1..12, b over 1..4 with |b| < 2 (denominators grow up to 4x per step); n = 500,000; jobs chain; K = 8; rows pg, pg-null, mpq, fmpq, fz
- **R1k-U1** (reuse): 300,000 operations over 600,000 values, each value an operand of 1 operation (entry-heavy): |num| < 10^6 over 1..1000; add and mul alternately, partners at random; operations = 300,000; jobs mix; rows pg, pg-null, mpq, fmpq
- **R1k-U3** (reuse): 300,000 operations over 200,000 values, each value an operand of 3 operations (reuse-heavy): |num| < 10^6 over 1..1000; add and mul alternately, partners at random; operations = 300,000; jobs mix; rows pg, pg-null, mpq, fmpq
- **R1k-U10** (reuse): 300,000 operations over 60,000 values, each value an operand of 10 operations (reuse-heavy): |num| < 10^6 over 1..1000; add and mul alternately, partners at random; operations = 300,000; jobs mix; rows pg, pg-null, mpq, fmpq
- **R1k-U30** (reuse): 300,000 operations over 20,000 values, each value an operand of 30 operations (reuse-heavy): |num| < 10^6 over 1..1000; add and mul alternately, partners at random; operations = 300,000; jobs mix; rows pg, pg-null, mpq, fmpq
- **R1k-U100** (reuse): 300,000 operations over 6,000 values, each value an operand of 100 operations (reuse-heavy): |num| < 10^6 over 1..1000; add and mul alternately, partners at random; operations = 300,000; jobs mix; rows pg, pg-null, mpq, fmpq
- **Rpq-U1** (reuse): 300,000 operations over 600,000 values, each value an operand of 1 operation (entry-heavy): |num| < 10^6 over 2,000 p*q (primes in [2^16, 2^31) from a pool of 64: 32-62 bits); add of two values over the same denominator; operations = 300,000; jobs add; rows pg, pg-null, mpq, fmpq
- **Rpq-U3** (reuse): 300,000 operations over 200,000 values, each value an operand of 3 operations (reuse-heavy): |num| < 10^6 over 2,000 p*q (primes in [2^16, 2^31) from a pool of 64: 32-62 bits); add of two values over the same denominator; operations = 300,000; jobs add; rows pg, pg-null, mpq, fmpq
- **Rpq-U10** (reuse): 300,000 operations over 60,000 values, each value an operand of 10 operations (reuse-heavy): |num| < 10^6 over 2,000 p*q (primes in [2^16, 2^31) from a pool of 64: 32-62 bits); add of two values over the same denominator; operations = 300,000; jobs add; rows pg, pg-null, mpq, fmpq
- **Rpq-U30** (reuse): 300,000 operations over 20,000 values, each value an operand of 30 operations (reuse-heavy): |num| < 10^6 over 2,000 p*q (primes in [2^16, 2^31) from a pool of 64: 32-62 bits); add of two values over the same denominator; operations = 300,000; jobs add; rows pg, pg-null, mpq, fmpq
- **Rpq-U100** (reuse): 300,000 operations over 6,000 values, each value an operand of 100 operations (reuse-heavy): |num| < 10^6 over 2,000 p*q (primes in [2^16, 2^31) from a pool of 64: 32-62 bits); add of two values over the same denominator; operations = 300,000; jobs add; rows pg, pg-null, mpq, fmpq
