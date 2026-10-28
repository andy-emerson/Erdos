# M3 peer comparison: pre-registered decision rule

Oct 28, 2026 08:34 CDT · written by `test/m3/m3_bench.py --write-rule` before any measurement of M3.  The measured run refuses to start unless this file contains the rule below verbatim.

**Revised Oct 28, 2026 08:34 CDT: revised after pre-run review.**  Nothing had been measured; this text supersedes the rule first fixed Oct 28, 2026 08:25 CDT.

**Decision rule for M3 (fixed before any measurement: in research/m3-rule.md, and in the header of results/m3.md; the
harness refuses a measured run unless research/m3-rule.md holds this text).**

1. **Rounds.**  Each job runs 12 interleaved rounds.  In every round each row of the job (M1, M1's null copy, and
   each library contender) runs once, as its own process, in an order rotated by one row per round, so the rows of a
   round are paired.  Each invocation runs 3 reps in one process; the first rep (cold) is discarded.  A row's time
   in a round is its **total**: entry (conversion from the raw int64 inputs; for M1 including creating its graph) +
   operation + exit (every result read out in lowest terms as two int64), the minimum of one rep's sum over the
   remaining 2 reps.  Entry is reported separately and is included in the total.
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
   check with the lowest T.  The job's verdict is M1's verdict against the best library.  (Taking the minimum of two or
   three rows biases this verdict toward the libraries; each library's own verdict is reported too.)
7. **Summary answers.**  (a) **Where is M1 faster than the best library:** the jobs whose verdict is faster.
   (b) **Where is it slower:** the jobs whose verdict is slower.  (Ties and n/a are listed too.)
   (c) **Pay-back of entry, on two axes,** each in two families: **mid** (|num| < 10^6, denominators in [2, 2^20):
   factored by trial division; add and mul alternately, partners at random) and **pq** (|num| < 10^6, denominators p*q
   of two primes in [2^16, 2^31): Pollard rho at M1's entry; add of two values over the same denominator).  Within a
   family the number of operations is fixed and only the axis variable changes.
   - **Axis A, uses per value U** over {1, 3, 10, 30, 100}: each value is an operand of exactly U operations, and each distinct
     denominator is carried by exactly 10 values, so distinct denominators per value stay fixed (1/10) and the
     pool of denominators grows with the number of values.
   - **Axis B, uses per distinct denominator V** over {2, 6, 20, 60, 200}: each value is an operand of exactly one operation, and
     each distinct denominator is carried by exactly V values (so used V times).
   On each axis and family, M1's entry cost **pays back from** the smallest grid value x at which the verdict is faster
   at x and at every larger value of the grid; it reaches **parity from** the smallest such x with tie or faster.  If the
   verdict at the largest value is not faster (resp. tie or faster), the answer is "not within the grid".  An n/a job
   breaks the run of values above it.
8. **Calibration: each contender gets its best normal use**, chosen on a separate calibration run before the rounds
   (3 invocations of each form, alternating; the form with the lower median total).  For fma and chain jobs: M1's
   array form, V4 (inside pg_expr_begin / pg_expr_end) or V3 (each call reduced on its own), used by M1 and its null
   copy alike; and FLINT fmpq's form of a*b + c, fmpq_mul + fmpq_add or fmpq_set + fmpq_addmul.  Calibration times are
   reported but carry no verdict.  No other comparison carries a verdict.


## Run plan

12 rounds, 3 reps per invocation (the first discarded), seed 20261028, 10,000 bootstrap resamples, band floor 5%.  Rows: pg, pg-null, mpq, fmpq, fz (fz only where the inputs have a shared denominator below 2^63).  Workloads:

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
- **A-mid-U1** (reuse, axis A (uses per value)): 300,000 operations over 600,000 values and 60,000 distinct denominators, each value an operand of 1 operation, each denominator on 10 values: |num| < 10^6 over distinct denominators in [2, 2^20) (trial division); add and mul alternately, partners at random; operations = 300,000; jobs mix; rows pg, pg-null, mpq, fmpq
- **A-mid-U3** (reuse, axis A (uses per value)): 300,000 operations over 200,000 values and 20,000 distinct denominators, each value an operand of 3 operations, each denominator on 10 values: |num| < 10^6 over distinct denominators in [2, 2^20) (trial division); add and mul alternately, partners at random; operations = 300,000; jobs mix; rows pg, pg-null, mpq, fmpq
- **A-mid-U10** (reuse, axis A (uses per value)): 300,000 operations over 60,000 values and 6,000 distinct denominators, each value an operand of 10 operations, each denominator on 10 values: |num| < 10^6 over distinct denominators in [2, 2^20) (trial division); add and mul alternately, partners at random; operations = 300,000; jobs mix; rows pg, pg-null, mpq, fmpq
- **A-mid-U30** (reuse, axis A (uses per value)): 300,000 operations over 20,000 values and 2,000 distinct denominators, each value an operand of 30 operations, each denominator on 10 values: |num| < 10^6 over distinct denominators in [2, 2^20) (trial division); add and mul alternately, partners at random; operations = 300,000; jobs mix; rows pg, pg-null, mpq, fmpq
- **A-mid-U100** (reuse, axis A (uses per value)): 300,000 operations over 6,000 values and 600 distinct denominators, each value an operand of 100 operations, each denominator on 10 values: |num| < 10^6 over distinct denominators in [2, 2^20) (trial division); add and mul alternately, partners at random; operations = 300,000; jobs mix; rows pg, pg-null, mpq, fmpq
- **A-pq-U1** (reuse, axis A (uses per value)): 30,000 operations over 60,000 values and 6,000 distinct denominators, each value an operand of 1 operation, each denominator on 10 values: |num| < 10^6 over distinct p*q, primes in [2^16, 2^31) from a pool of 512 (32-62 bits; Pollard rho at M1's entry); add of two values over the same denominator; operations = 30,000; jobs add; rows pg, pg-null, mpq, fmpq
- **A-pq-U3** (reuse, axis A (uses per value)): 30,000 operations over 20,000 values and 2,000 distinct denominators, each value an operand of 3 operations, each denominator on 10 values: |num| < 10^6 over distinct p*q, primes in [2^16, 2^31) from a pool of 512 (32-62 bits; Pollard rho at M1's entry); add of two values over the same denominator; operations = 30,000; jobs add; rows pg, pg-null, mpq, fmpq
- **A-pq-U10** (reuse, axis A (uses per value)): 30,000 operations over 6,000 values and 600 distinct denominators, each value an operand of 10 operations, each denominator on 10 values: |num| < 10^6 over distinct p*q, primes in [2^16, 2^31) from a pool of 512 (32-62 bits; Pollard rho at M1's entry); add of two values over the same denominator; operations = 30,000; jobs add; rows pg, pg-null, mpq, fmpq
- **A-pq-U30** (reuse, axis A (uses per value)): 30,000 operations over 2,000 values and 200 distinct denominators, each value an operand of 30 operations, each denominator on 10 values: |num| < 10^6 over distinct p*q, primes in [2^16, 2^31) from a pool of 512 (32-62 bits; Pollard rho at M1's entry); add of two values over the same denominator; operations = 30,000; jobs add; rows pg, pg-null, mpq, fmpq
- **A-pq-U100** (reuse, axis A (uses per value)): 30,000 operations over 600 values and 60 distinct denominators, each value an operand of 100 operations, each denominator on 10 values: |num| < 10^6 over distinct p*q, primes in [2^16, 2^31) from a pool of 512 (32-62 bits; Pollard rho at M1's entry); add of two values over the same denominator; operations = 30,000; jobs add; rows pg, pg-null, mpq, fmpq
- **B-mid-V2** (reuse, axis B (uses per distinct denominator)): 300,000 operations over 600,000 values and 300,000 distinct denominators, each value an operand of one operation, each denominator on 2 values (used 2 times): |num| < 10^6 over distinct denominators in [2, 2^20) (trial division); add and mul alternately, partners at random; operations = 300,000; jobs mix; rows pg, pg-null, mpq, fmpq
- **B-mid-V6** (reuse, axis B (uses per distinct denominator)): 300,000 operations over 600,000 values and 100,000 distinct denominators, each value an operand of one operation, each denominator on 6 values (used 6 times): |num| < 10^6 over distinct denominators in [2, 2^20) (trial division); add and mul alternately, partners at random; operations = 300,000; jobs mix; rows pg, pg-null, mpq, fmpq
- **B-mid-V20** (reuse, axis B (uses per distinct denominator)): 300,000 operations over 600,000 values and 30,000 distinct denominators, each value an operand of one operation, each denominator on 20 values (used 20 times): |num| < 10^6 over distinct denominators in [2, 2^20) (trial division); add and mul alternately, partners at random; operations = 300,000; jobs mix; rows pg, pg-null, mpq, fmpq
- **B-mid-V60** (reuse, axis B (uses per distinct denominator)): 300,000 operations over 600,000 values and 10,000 distinct denominators, each value an operand of one operation, each denominator on 60 values (used 60 times): |num| < 10^6 over distinct denominators in [2, 2^20) (trial division); add and mul alternately, partners at random; operations = 300,000; jobs mix; rows pg, pg-null, mpq, fmpq
- **B-mid-V200** (reuse, axis B (uses per distinct denominator)): 300,000 operations over 600,000 values and 3,000 distinct denominators, each value an operand of one operation, each denominator on 200 values (used 200 times): |num| < 10^6 over distinct denominators in [2, 2^20) (trial division); add and mul alternately, partners at random; operations = 300,000; jobs mix; rows pg, pg-null, mpq, fmpq
- **B-pq-V2** (reuse, axis B (uses per distinct denominator)): 30,000 operations over 60,000 values and 30,000 distinct denominators, each value an operand of one operation, each denominator on 2 values (used 2 times): |num| < 10^6 over distinct p*q, primes in [2^16, 2^31) from a pool of 512 (32-62 bits; Pollard rho at M1's entry); add of two values over the same denominator; operations = 30,000; jobs add; rows pg, pg-null, mpq, fmpq
- **B-pq-V6** (reuse, axis B (uses per distinct denominator)): 30,000 operations over 60,000 values and 10,000 distinct denominators, each value an operand of one operation, each denominator on 6 values (used 6 times): |num| < 10^6 over distinct p*q, primes in [2^16, 2^31) from a pool of 512 (32-62 bits; Pollard rho at M1's entry); add of two values over the same denominator; operations = 30,000; jobs add; rows pg, pg-null, mpq, fmpq
- **B-pq-V20** (reuse, axis B (uses per distinct denominator)): 30,000 operations over 60,000 values and 3,000 distinct denominators, each value an operand of one operation, each denominator on 20 values (used 20 times): |num| < 10^6 over distinct p*q, primes in [2^16, 2^31) from a pool of 512 (32-62 bits; Pollard rho at M1's entry); add of two values over the same denominator; operations = 30,000; jobs add; rows pg, pg-null, mpq, fmpq
- **B-pq-V60** (reuse, axis B (uses per distinct denominator)): 30,000 operations over 60,000 values and 1,000 distinct denominators, each value an operand of one operation, each denominator on 60 values (used 60 times): |num| < 10^6 over distinct p*q, primes in [2^16, 2^31) from a pool of 512 (32-62 bits; Pollard rho at M1's entry); add of two values over the same denominator; operations = 30,000; jobs add; rows pg, pg-null, mpq, fmpq
- **B-pq-V200** (reuse, axis B (uses per distinct denominator)): 30,000 operations over 60,000 values and 300 distinct denominators, each value an operand of one operation, each denominator on 200 values (used 200 times): |num| < 10^6 over distinct p*q, primes in [2^16, 2^31) from a pool of 512 (32-62 bits; Pollard rho at M1's entry); add of two values over the same denominator; operations = 30,000; jobs add; rows pg, pg-null, mpq, fmpq
