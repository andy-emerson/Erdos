# Design 3 on matrix multiplication: results

Run started 2026-10-30T00:43:47-05:00.

## The measurement rule (research/rule-m1.md, verbatim, written before the first trial)

sha256 `8e951ac6597f099962aa77580653e799d101af6d7b3e30a51e1a6005ab7aeff7`

<!-- rule begins -->
# Design 3 on matrix addition and multiplication: measurement rule

Oct 29, 2026 · **Fixed: approved by the author, Oct 29, 2026**, after three independent reviews whose fixes are applied. Nothing is built, run or measured. It follows the author's decisions of Oct 29, 2026 (spec v0.1 §0), including the later ones: the index store is dropped; $A$ and $B$ are 64-bit in every build; the core is 64-bit; two table sizings, fixed and grow, are tested, and grow's two rewrite methods, lookup and map; the four pairings against TC; matrix addition, run first as its own run; $n = 8$ dropped; all 16 jobs kept in both sets, with the hard stop raised to 40 minutes and the pilot's projection limit to 30. Each threshold is either **carried over** from round 3's fixed rule ([`../../10-interning/research/rule-r3.md`](../../10-interning/research/rule-r3.md), with its revision; approved by the author, Oct 29, 2026), and marked so, or new and **approved by the author, Oct 29, 2026**, and marked so. As in round 3, the harness refuses a measured run unless this file has the approved text (by sha256, pinned in the harness), and while any mark of a proposal awaiting approval remains in it. A change after the first trial is a revision, dated and explained at the top.

This rule measures the six builds of [spec v0.1](../v0.1.md), with every structure counted (spec §3); the counts are the same for both operations:
- **TC**: 3 structures, one array of 64-bit pairs per matrix;
- **Hash-V-fixed**: 6, the cells and a table of $T_\text{fix}$ slots, per matrix;
- **Hash-M-fixed**: 7, Hash-V-fixed's and the translation $\tau$ from $B$'s handles to $A$'s (new, approved by the author for testing);
- **Hash-V-grow**: 6, the cells and a growing table, per matrix; cells rewritten by lookup (the replacement table is the same structure);
- **Hash-M-grow**: 7, Hash-V-grow's and $\tau$;
- **Hash-V-grow-map**: 7, Hash-V-grow's and the slot map (r), a **new** per-call temporary that exists only during a growth.

All six use the same 64-bit core (the 08 module) and the same inputs at the same width. Their arithmetic is identical, call for call (spec §10 item 3). Results are reported **per measure, with no overall winner**.

**Two runs (the author's decision).** The rule applies unchanged to two job sets (Workloads): the **addition run** first, then, only after the author has seen its results, the **multiplication run**. Each run has its own steps 0–7 (Run plan): its own pilot and projection, its own hard stop, its own results folder and its own report. Nothing measured in one run enters a measure of the other.

## Measurement rule

1. **Rows.** Each job runs seven rows: the six builds and **Hash-V-fixed-null**, Hash-V-fixed's binary run as a separate row for the null control (item 8) (approved by the author, Oct 29, 2026: the plainest hash row, as round 3's null row was a copy of its hash row).

2. **Builds.** As rule-r3 item 2 (carried over): `gcc -std=c11 -O2` (gcc, for the 08 module's `__builtin_*_overflow`); the 08 module and `../../10-interning/lib/src/tc3.c` unchanged, their sha256 checked before every run; TC built from the new `../lib/src/tcm.c`, whose diff from `tc3.c` changes only where results go (spec §4), checked by a recorded diff; one counting run and one memory run per row and job; a limit of **10 minutes** per counting or memory run. Three builds per row: timing, counting and test (approved by the author, Oct 29, 2026: no profiling build, spec M-9). The memory build reads the heap inside each growth (spec §9). The same binaries serve both runs; the job decides the operation. Repeats on jobs **1 and 6** of each set MUST be identical (approved by the author, Oct 29, 2026: a small and a middle job, as round 3 used 3 and 10).

3. **Gates before the measured run.** As rule-r3 item 3 (carried over), for each row: the unit tests of spec §10, and on every job of the run's set the gate and the exact checks of spec §10 items 1–7. The counting runs MUST pass the call-for-call check: every hash row's per-call arithmetic equal to TC's. **The TC gate** (approved by the author, Oct 29, 2026, spec §4): on every job, `tcm.c`'s case and gcd counts (gcds, gcd work, exact divisions, multiplications, additions) MUST equal `tc3.c`'s on the same operations, operation for operation. Gates MAY run two at a time, only before the timed steps.

4. **Timing.** As rule-r3 item 4, every number carried over: **12 interleaved rounds**, the order rotated by one row per round over the seven rows; **r**, the smallest power of two with a rep of at least **50 ms**; **3 reps** per invocation, the first discarded and the minimum of the other two kept, or **1 rep** if the calibration rep lasts at least **1 s**; calibration per job just before round 1 for the six builds, the null row taking Hash-V-fixed's r; pinning to **core 1**, the runner on core 0; a cap of **10 s** per rep; the process's own CPU time, reported only.

   **The parts** (spec §8): **creation** (`create` of $A$, $B$, $C$, every entering call of $A$ and $B$, and, in the grow rows, every growth of $A$ and $B$ with its reinsertion and rewrite); **operation** (the addition or the multiply: $\tau$ in the M rows, every addition and, in multiplication, every product, and the stores of $C$ with its widenings and, in the grow rows, its growths); **read-out**. $\tau$'s build time is also reported alone, with its share of the operation part, with no verdict. A row's time in a round is each part's time per run; the **total** is their sum.

5. **Nothing of the runner's own runs during a timed phase.** **Carried over in full** from rule-r3 item 5 (itself rule-r2 item 5 with both revisions): the single-threaded runner; each timed invocation started by `fork` and `exec`, with `sched_setaffinity` to core 1 and `dup2` to a file before `exec`; one blocking `waitpid`; the kernel timer for the hard stop; no writes during an invocation; the operator starts the runner once per run, as one background command with a timeout of at least **45 minutes** (the hard stop plus 5, as round 3's 35; approved by the author, Oct 29, 2026), and runs nothing else until it exits. **The disturbance check by process**, after each timed block, with every number: other processes at most $5 + \lceil 1 \cdot L \rceil$ ticks; a new process under 1 tick; hosts held to $5 + \lceil 2\,R_h\,L \rceil$, with $R_h$ from the run's own pilot and at least 1 tick over its longest block; a stop above **4 ticks per second** for any host; a disturbed block re-run once, a second disturbance stopping the run. **The runner's ticks** reported in total and per launch, with the sanity limit of more than **1 tick per launch plus 2** per block.

6. **Idle machine.** Carried over: a **2-second** sample before each timed block; idle when no process outside the runner used more than **5%** of one core and core 1 was at most **2%** busy; if busy, re-check every **15 s**, stop after **10 minutes**.

7. **Measures**, per job, for each **pairing**, in each run (9 pairings, all approved by the author, Oct 29, 2026):
   - **against TC** (4): Hash-V-fixed, Hash-M-fixed, Hash-V-grow, Hash-M-grow (**approved by the author, Oct 29, 2026**). TC is the baseline. There is no single textbook hash, so each hash row is compared with TC on its own, and the hash rows are compared with each other (below);
   - **M against V**, within each sizing (2): Hash-M-fixed against Hash-V-fixed; Hash-M-grow against Hash-V-grow;
   - **grow against fixed**, within V and within M (2): Hash-V-grow against Hash-V-fixed; Hash-M-grow against Hash-M-fixed;
   - **map against lookup** (1): Hash-V-grow-map against Hash-V-grow, **on creation only**: a creation-time verdict and the growth peak (below); its operation time (which holds $C$'s growths) and total are reported without a verdict, and its end memory, equal by construction since (r) is freed, is reported only.

   **The hash-against-hash pairings each isolate one change:** M against V (the cross test), grow against fixed (the sizing), map against lookup (the rewrite).

   The measures, as rule-r3 item 7 (carried over), except where marked:
   - **Creation time:** T_c(reference)/T_c(row), T_c the median over rounds of the creation part per run; above 1 the row is faster; 95% interval from **10,000 paired bootstrap resamples** of the rounds, seed 20261029.
   - **Operation time:** the same, on the operation part.
   - **Total and read-out time:** ratio and interval, **no verdict**.
   - **Bytes per cell, accounted (B_used):** TC 16; hash rows as spec §9, over the $3n^2$ cells, tables counted in full, $\tau$ included in the M rows. **Fixed loses by design** (spec §9): in multiplication Hash-V-fixed 2.25–2.5 times TC and Hash-M-fixed ($\tau$ included) 2.42–2.83 times; in addition, where $C$'s table is 32-bit, Hash-V-fixed 1.92–2.17 times and Hash-M-fixed 2.08–2.5 times. So the fixed rows carry the severe-memory-loss flag on every job except addition's Hash-V-fixed at $n \le 64$; this is reported as expected. **Grow is the data-dependent one:** its tables follow $W$, not the bound.
   - **Bytes per cell, heap (B_heap):** the growth of `mallinfo2` (`uordblks + hblkhd`) from just before the first `create`, over $3n^2$, with `mallopt` as round 3. Both memory figures after creation and at the end of the operation (before $\tau$ is freed); verdicts use the end figures.
   - **Growth peak** (approved by the author, Oct 29, 2026; grow rows): the most bytes, accounted and heap, at any growth (old table, its replacement and, in Hash-V-grow-map, (r), with the other matrices' bytes), per cell; reported for every grow row and as a ratio in the map-against-lookup pairing, **no verdict**.
   - **Gcd work: a check, not a verdict.** G(row)/G(TC) MUST be exactly 1 for every hash row. Any other value is a bug.

8. **Null control.** As rule-r3 item 8 (carried over), per run: Hash-V-fixed against Hash-V-fixed-null gives a null spread per time measure, max(1 − lo, hi − 1, 0); each time verdict uses **h = max(5%, that measure's null spread)**, the same h for every pairing of that run. **Caveat** (as round 3): the other rows' own noise is not measured; on jobs where a row runs 1 rep per invocation its noise may differ, and the report marks those verdicts "(c)".

9. **Verdicts per job, pairing and measure, with magnitudes.** Carried over:

   | Measure | Row wins | Row loses | Tie | Magnitude reported |
   | --- | --- | --- | --- | --- |
   | Creation time | lo > 1 + h_c | hi < 1 − h_c | otherwise | ratio, [lo, hi], h_c |
   | Operation time | lo > 1 + h_o | hi < 1 − h_o | otherwise | ratio, [lo, hi], h_o |
   | B_used (end) | ratio ≤ 0.90 | ratio ≥ 1.10 | otherwise | ratio after creation and at the end; $W$, $T$, fill, $w$, $w_h$ per store; growths |
   | B_heap (end) | ratio ≤ 0.90 | ratio ≥ 1.10 | otherwise | ratio after creation and at the end |

   A memory ratio of at least **2.0** raises the severe-memory-loss flag. A row over cap has the verdict **over cap** on both time measures. A verdict is never reported without its magnitude.

10. **No profile in this round** (approved by the author, Oct 29, 2026, spec M-9).

11. **Strata, and no overall winner.** Approved by the author, Oct 29, 2026, per run: the strata are the four sharing levels of $A$ and $B$, **NONE** ($\rho = 1$), **LOW** ($\rho = 4$), **MID** ($\rho = 16$), **HIGH** ($\rho = 64$). $\rho$ sets the sharing of rationals in $A$ and $B$ only.
    - **$C$'s sharing:** in multiplication, $C$'s cells are sums of $n$ products and are nearly all distinct at every $\rho$. In addition, $C$ has at most $\min(n^2, s^2)$ distinct values, so it is shared where $s$ is small against $n$ ($s \le n$: $n = 16$ at $\rho = 16$ and 64, $n = 32$ and 64 at $\rho = 64$; at $n = 16$, $\rho = 64$, at most 16 distinct values in 256 cells) and nearly unshared elsewhere.
    - **$\rho = 1$ is not the worst case for whole numbers:** numerator magnitudes are at most 32,767 and there are 30 denominators, so $W_A$ is about 12,000 at $n = 128$, not $2n^2 + 2 = 32{,}770$.

    For each stratum, pairing and measure: counts of wins, ties, losses, over-cap and n/a, the median ratio, and the severe flags. Also, per pairing and time measure, a **size × sharing grid** of ratios, presented, not judged. The rule combines no measures, names no better row, and does not combine the two runs.

12. **Checks, and bugs after the gates.** As rule-r3 item 12 (carried over). Every cell of every invocation is checked against `fractions`. After the gates each of these is a **bug**: a check failure; an overflow (64 bits, in any build); a failed exact check of spec §10; a non-repeating count; a full table; a grow table over half full; gcd work differing between rows; two hash rows taking different cases. A fix to `tcm.c` re-runs TC; a fix to the hash source re-runs every hash row and the null row. A fix made after the addition run and before the multiplication run is a revision of the code, recorded; the addition results stand as measured.

13. **Also reported:** per job and store ($A$, $B$, $C$), after creation and at the end: $W$, $T$, fill $W/T$, $w$, average probes per store, longest store, $C$'s widenings and slots copied; in the grow rows, growths, whole numbers reinserted, reinsertion probes, cell fields rewritten and, for lookup, rewrite probes; for the M rows, $\tau$'s finds (found, none), their probes, and its build time; realized sharing ($s$, $W$ per store); cases per operation; cross tests and cross reads per operation, and how many cross tests matched; the idle and disturbance checks with the runner's ticks per launch; the process's own CPU time. **Round 3, for reference only:** a link to its results; they enter no measure here. The multiplication report links the addition report, for reference only.

## Workloads (approved by the author, Oct 29, 2026)

**Generator.** New, `test/m1_gen.py`. Two job sets, **addition** (jobs `add-1` to `add-16`) and **multiplication** (`mul-1` to `mul-16`), with the same sizes, sharing levels and value distribution. Master seed 20261029; job $j$ of the addition set seeded 20261029 + $j$, of the multiplication set 20261029 + 100 + $j$, each drawing $A$'s values, then $B$'s, from one stream. **Order of entry** (spec §5.7): every build enters $A$'s cells in row-major order, then $B$'s, the numerator before the denominator in each cell, and fills $C$ row-major during the operation; the model's slots and growths follow this order. Every build reads the same workload file.
- **A value:** numerator uniform in $[-32767, 32767]$, denominator uniform among the 30 divisors of 720, reduced. *Why:* small inputs keep every 64-bit intermediate far from overflow (below). A store widens straight to the smallest width that holds the value (spec §5.4), so $C$ widens in its first few stores, one to three copies per job: in **addition** to 32 bits, possibly by way of 16 (a 16-bit step when its first values fit 16 bits; the exact model predicts every step and the gates check it), ($C$'s numerators reach up to $2 \cdot 32767 \cdot 720 \approx 4.7 \times 10^7$, 26 bits); in **multiplication** from 8 to 64 bits ($C$'s whole numbers typically 40–50 bits, about 56 at worst). In multiplication the 16- and 32-bit steps are exercised only by the unit tests.
- **Sharing:** a pool of $s = n^2/\rho$ distinct values per matrix ($\rho$ cells per distinct value); each pool value is placed once and the other cells drawn uniformly from the pool, then the cells are shuffled, so each matrix has exactly $s$ distinct values. $A$ and $B$ draw separate pools. **Every job has at least 4 distinct values:** the fewest is $s = 16^2/64 = 4$ (job 4 of each set); every other job has $s \ge 16$.
- **Every 64-bit intermediate fits, in every build.** Each factor's denominator divides 720, so every sum, product and partial sum has a denominator dividing $720^2 = 518{,}400$.
  - **Addition** $a/b + c/d$ with $g = \gcd(b, d)$: each term $a (d/g)$ and $c (b/g)$ is at most its operand's magnitude times $\text{lcm}(b, d) \le 518{,}400$, and their sum is the result's magnitude times the same. In an addition job the operands are at most 32,767 in magnitude, so every numerator, term and sum is at most $2 \cdot 32767 \cdot 518{,}400 \approx 3.4 \times 10^{10}$, and every denominator at most $518{,}400$.
  - **Multiply**, gcd-first: the numerator is at most $|a|\,|c| \le 32767^2 \approx 1.07 \times 10^9$ and the denominator at most $b\,d \le 518{,}400$. A partial sum of at most $n$ products has magnitude at most $n \cdot 32767^2$, so in its additions every numerator, term and sum is at most $n \cdot 32767^2 \cdot 518{,}400 \approx 7.1 \times 10^{16}$ for $n = 128$.
  - Both bounds are below $2^{63} \approx 9.2 \times 10^{18}$, by factors of about $2.7 \times 10^8$ (addition) and 130 (multiplication), so no intermediate overflows and none equals $-2^{63}$. **The value ranges do not need to shrink.** The model checks every intermediate anyway; an overflow after the gates is a bug (item 12).

**The sweep**, the same for both sets: square matrices, $n \in \{16, 32, 64, 128\}$ by $\rho \in \{1, 4, 16, 64\}$: **16 jobs per set**.
- $n = 16$: the smallest, where `create` and the tables' emptying weigh most against the operation.
- $n = 32$: a doubling step; addition grows as $n^2$, multiplication as $n^3$, stores as $n^2$.
- $n = 64$: the largest with 16-bit handles.
- $n = 128$: the first with 32-bit handles. **Confound:** from 64 to 128 both size and $w_h$ change (16 → 32 bits), so that step does not separate them.
- $\rho = 1$: no repeated rationals in $A$ and $B$. Not the worst case for whole numbers, which repeat anyway (item 11).
- $\rho = 4$, $16$, $64$: each step four times the reuse; at $\rho = 64$, $n = 16$, a matrix has 4 distinct values, and at $n = 128$, 256.

| $n$ | $\rho = 1$ | $\rho = 4$ | $\rho = 16$ | $\rho = 64$ |
| --- | --- | --- | --- | --- |
| 16 | job 1, $s$ = 256 | 2, 64 | 3, 16 | 4, 4 |
| 32 | 5, 1,024 | 6, 256 | 7, 64 | 8, 16 |
| 64 | 9, 4,096 | 10, 1,024 | 11, 256 | 12, 64 |
| 128 | 13, 16,384 | 14, 4,096 | 15, 1,024 | 16, 256 |

*Informed by round 3* (non-normative): its hash row cost about 0.7–0.8 of TC's speed per operation, with every result stored. Addition stores every result, like round 3, but with operands from two stores; in multiplication the $n^3$ products are temporaries and only the $n^2$ cells of $C$ are stored, so the two runs show interning's cost and saving when stores are frequent and when they are rare against arithmetic.

## Run plan (new steps approved by the author, Oct 29, 2026), once per run

The addition run makes steps 0–7 on the addition set. The multiplication run makes them again on the multiplication set, after the author has seen the addition report. Each run has its own clock, pilot and results folder: **`11-matrix/test/results-m1-add/`** and **`11-matrix/test/results-m1-mul/`**.

0. **Before the clock** (approved by the author, Oct 29, 2026): generate the run's workloads, the `fractions` references and the model's predictions (both sizings and both rewrite methods), untimed and outside the hard stop, and record each file's sha256. Addition: well under a minute. Multiplication: Python over about $2.1 \times 10^6$ products and as many additions per $n = 128$ job, about 3–5 minutes.
1. Build every row in its three builds, check the hashes of the 08 module and `tc3.c`, record `tcm.c`'s diff, and check the sha256 of step 0's files. (The multiplication run checks that the binaries are the addition run's, or records why not.)
2. Unit tests (test build, and a sanitizer build).
3. **Pilot:** jobs **1, 6 and 13** of the run's set, every row, r calibration and **2 rounds**, one block per job (6 + 14 = 20 launches). It records $R_h$ and each block's wall time, idle and disturbance checks included: $P_6$ (job 6, $n = 32$) and $P_{13}$ (job 13, $n = 128$). **Stop rule**, the same form in both runs:
   $$\text{projection} = \text{elapsed} + 1.2 \times 5 \times (N_{128}\,P_{13} + N_\text{rest}\,P_6),$$
   with $N_{128}$ the run's jobs at $n = 128$ and $N_\text{rest}$ its other jobs (4 and 12 for the full sets). *Why this form:* each job is projected from a pilot block of its own kind, its fixed floor included (reps of at least 50 ms, 90 launches, the idle check and the disturbance check): the $n = 128$ jobs from $P_{13}$, the others from $P_6$ (floor-dominated; $n = 16$ costs a little less and $n = 64$ a little more). **5** scales a pilot block (20 launches) to a full one (90 launches, $90/20 = 4.5$), taking the checks as if they scaled too; **1.2** adds the gates, counting and memory runs. **If the projection exceeds 30 minutes (approved by the author, Oct 29, 2026; round 3's was 22), the run stops** and is reported to the author. It also stops if any host's rate exceeds 4 ticks per second (carried over).
4. Gates, two at a time.
5. Counting and memory runs, two at a time; then the worker pool is joined.
6. Per job, in job order: the idle check, calibration (6 launches) and the 12 rounds (84 launches), then the disturbance check.
7. Analysis and report, untimed.

**Hard stop at 40 minutes (approved by the author, Oct 29, 2026; round 3's was 30), per run,** from step 1 to the end of step 6, enforced by the kernel timer. A stopped run reports no verdict.

### Runtime estimate (non-normative)

From round 3: TC about 60–80 ns per operation, the hash row about 1.2–1.4 times that. Every build runs the same 64-bit core, so the hash rows differ from TC only in reading cells by handle, the cross tests, $\tau$, the stores and the growths (milliseconds per job). A launch costs at least its floor: three reps of at least 50 ms, with the process start and the workload read, about 0.25 s.

**Addition run.** At $n = 128$ a run is about $2n^2$ entries and $n^2$ additions with their stores, a few milliseconds, so every launch is at its floor: about 0.25–0.3 s.
- Per job block (90 launches, checks about 3 s): about 25–30 s; 16 jobs, about 7–8 minutes.
- Build, unit tests and pilot about 2 minutes; gates, counting and memory runs about 2.
- **On the clock: about 11–12 minutes**, against the 40-minute hard stop (margin about 3.3).
- **Pilot projection:** $P_6 \approx 8$ s, $P_{13} \approx 9$ s: $6 \times (4 \cdot 9 + 12 \cdot 8) \approx 13$ minutes, plus about 1.5 elapsed: **about 15 minutes**, against the 30-minute limit (margin about 2).

**Multiplication run.** At $n = 128$, TC about 0.3 s per run and the hash rows 0.35–0.45 s, so about 1.0 s (TC) and 1.2–1.4 s (hash) per 3-rep launch; at $n \le 64$, about 0.25 s per launch, floor-dominated.
- Per job block: about 1.9 minutes at $n = 128$; about 25 s otherwise. 16 jobs: about $4 \times 1.9 + 12 \times 0.42 \approx 12.6$ minutes.
- Build, unit tests and pilot about 2.5 minutes; gates, counting and memory runs about 3.5.
- **On the clock: about 18–19 minutes**, against the 40-minute hard stop (margin about 2.1).
- **Pilot projection:** $P_6 \approx 8$ s, $P_{13} \approx 28$ s: $6 \times (4 \cdot 28 + 12 \cdot 8) \approx 21$ minutes, plus about 2.5 elapsed: **about 23 minutes**, against the 30-minute limit (margin about 1.3). The projection would pass 30 minutes only if the $n = 128$ launches ran about 1.6 times slower than estimated ($P_{13} \approx 45$ s); the run would then take about 25 minutes, still inside the hard stop, and the pilot's stop would report it to the author first, as intended.
- **Step 0, outside the clock:** about 3–5 minutes. The longest rep is under 2 s, within the 10 s cap.

## Limitations, reported with the results

- **Textbook only.** No row is optimized: plain open addressing with linear probing; growth by doubling.
- **The index store is not tested** in this round (the author's decision).
- **Fixed's fill is low and not controlled:** its tables are sized from the dimensions ($T \ge 2n^2 + 2$), and it loses on memory by design. **Grow has a fill limit** of one half (spec §5.6, approved by the author, Oct 29, 2026), which the author rejected for round 3's fixed table.
- **The map method is tested on V only;** the rewrite method and V/M are taken to act on different parts (creation and operation), though $C$'s growths put a little of the rewrite into the operation part.
- **Addition is floor-dominated:** its runs take milliseconds, so its time ratios rest on reps of about 50 ms, made of many runs; the null band shows how far that holds.
- **Two runs, not one:** the runs are separate in time, so machine conditions can differ between them; their results are not combined.
- **64 bits only.** A value past 64 bits is an error in every build; the sweep stays far below.
- **Square matrices, one value distribution.** $A$ and $B$ have the same sharing level.
- Every other limitation of round 3 that applies (the disturbance check cannot see every process; a child of a process that ignores `SIGCHLD` is not charged).

## Harness

Round 3's harness (`../../10-interning/test/r3_*.py`, `bench_r3.c`) is the starting point, copied into `11-matrix/test/` as `m1_*` files; its spawner, idle and disturbance checks, calibration, bootstrap and stops are kept unchanged. New: the generator of the two job sets, and the model of both operations, the tables (both sizings, every growth by both methods), $C$'s widths and widenings, and $\tau$; the driver for the six builds, both operations and three parts, with the memory build's reads inside each growth; a run argument (`add` or `mul`) that selects the job set and the results folder; the analysis for nine pairings, the growth peak, the strata and the grid. Results go only to the run's own folder. The harness refuses to write anywhere in `../10-interning/`, and checks the sha256 of round 3's results before and after each run; the multiplication run also refuses to write in the addition run's folder, and checks its files' sha256 before and after. Builder decisions go in `11-matrix/test/DECISIONS.md`, pending the author's review.
<!-- rule ends -->

## The run

- The multiplication run (job set `mul`).
- Started 2026-10-30T00:43:47-05:00; finished 2026-10-30T00:56:44-05:00.
- Step 0 (before the clock): 88.4 s.
- Run time 11.5 minutes (hard stop 40).
- Rule sha256 `8e951ac6597f099962aa77580653e799d101af6d7b3e30a51e1a6005ab7aeff7`; round 3's results after the run: unchanged.
- Host processes (the runner's ancestors): bash[16282], claude[93], environment-man[85], sh[79], process_api[1]
- Binaries: the addition run's; the addition run's results after this run: unchanged.
- Code at 2026-10-30T00:45:15-05:00: TC `80e68597f321`, MHVF `86905b11a897`, MHMF `76c706ad7a52`, MHVG `0624617cc9f3`, MHMG `b0703662a2f2`, MHVGM `260bbcf2d702`, TC3GATE `1faeb759cc4c`
- Unit tests: m1_unit: m1_unit: 4014 checks, 0 failures; m1_diff: m1_diff: 3000 seeds, 3000 jobs, 747676 operations; 1591644 checks, 0 failures; m1_alloc-timing: m1_alloc (timing build): 180 jobs, 18312 checks, 0 failures; m1_alloc-counting: m1_alloc (counting build): 180 jobs, 18312 checks, 0 failures; m1_unit-asan: m1_unit: 4014 checks, 0 failures; m1_diff-asan: m1_diff: 600 seeds, 600 jobs, 151723 operations; 324084 checks, 0 failures

## The pilot (discarded, except what it records for the run)

- Jobs [1, 6, 13]; block wall times job 1 5.7 s, job 6 5.7 s, job 13 15.4 s; P_6 = 5.7 s, P_13 = 15.4 s; N_128 = 4, N_rest = 12.
- Projection elapsed + 1.2 x 5 x (N_128 P_13 + N_rest P_6) = 13.9 min (limit 30).
- Host rates R_h (ticks per second): {'1': 0.07461016190405133, '79': 0.07461016190405133, '85': 0.07461016190405133, '93': 0.07461290259634619, '16282': 0.07461016190405133}.

## Verdicts, per job, pairing and measure (no overall winner)

Time: T(reference)/T(row), above 1 the row is faster; creation and operation each have their own band h from Hash-V-fixed against Hash-V-fixed-null on that part. Total and read-out time are reported without a verdict. Bytes per cell: row over reference, below 1 the row is smaller (end of the operation; after creation in parentheses). Map against lookup is judged on creation only. Gcd work is a check, not a verdict. (c): the other rows' own noise is not measured: the null row is a copy of Hash-V-fixed, and on this job a row of the pairing ran 1 rep per invocation (its calibration rep lasted at least 1 s), so its noise may differ (rule-m1 item 8).

### Hash-V-fixed against TC (against TC)

| job | creation: verdict, ratio [lo, hi], h | operation: verdict, ratio [lo, hi], h | total (reported) | read-out (reported) | B_used (end): verdict, ratio (after creation) | B_heap (end): verdict, ratio (after creation) |
| --- | --- | --- | --- | --- | --- | --- |
| 1 (n 16, rho 1) | loss, 0.601 [0.593, 0.618], h 0.050 | loss, 0.883 [0.878, 0.889], h 0.050 | reported, 0.871 [0.866, 0.877] | reported, 0.604 [0.588, 0.692] | loss, 2.250 (1.667) **severe memory loss** | loss, 2.316 (1.663) **severe memory loss** |
| 2 (n 16, rho 4) | loss, 0.614 [0.601, 0.624], h 0.050 | loss, 0.875 [0.872, 0.880], h 0.050 | reported, 0.864 [0.861, 0.868] | reported, 0.599 [0.585, 0.704] | loss, 2.250 (1.667) **severe memory loss** | loss, 2.316 (1.663) **severe memory loss** |
| 3 (n 16, rho 16) | loss, 0.632 [0.622, 0.653], h 0.050 | loss, 0.840 [0.834, 0.845], h 0.050 | reported, 0.832 [0.826, 0.838] | reported, 0.602 [0.586, 0.617] | loss, 2.250 (1.667) **severe memory loss** | loss, 2.316 (1.663) **severe memory loss** |
| 4 (n 16, rho 64) | loss, 0.655 [0.641, 0.669], h 0.050 | loss, 0.849 [0.843, 0.856], h 0.050 | reported, 0.843 [0.836, 0.849] | reported, 0.604 [0.574, 0.633] | loss, 2.250 (1.667) **severe memory loss** | loss, 2.316 (1.663) **severe memory loss** |
| 5 (n 32, rho 1) | loss, 0.665 [0.661, 0.674], h 0.050 | loss, 0.896 [0.894, 0.901], h 0.050 | reported, 0.891 [0.889, 0.896] | reported, 0.651 [0.615, 0.784] | loss, 2.250 (1.667) **severe memory loss** | loss, 2.246 (1.666) **severe memory loss** |
| 6 (n 32, rho 4) | loss, 0.657 [0.651, 0.669], h 0.050 | loss, 0.897 [0.892, 0.899], h 0.050 | reported, 0.892 [0.887, 0.894] | reported, 0.669 [0.636, 0.831] | loss, 2.250 (1.667) **severe memory loss** | loss, 2.246 (1.666) **severe memory loss** |
| 7 (n 32, rho 16) | loss, 0.696 [0.684, 0.705], h 0.050 | loss, 0.894 [0.888, 0.911], h 0.050 | reported, 0.890 [0.884, 0.906] | reported, 0.665 [0.631, 0.677] | loss, 2.250 (1.667) **severe memory loss** | loss, 2.246 (1.666) **severe memory loss** |
| 8 (n 32, rho 64) | loss, 0.713 [0.708, 0.720], h 0.050 | loss, 0.882 [0.878, 0.887], h 0.050 | reported, 0.879 [0.875, 0.884] | reported, 0.639 [0.621, 0.761] | loss, 2.250 (1.667) **severe memory loss** | loss, 2.246 (1.666) **severe memory loss** |
| 9 (n 64, rho 1) | loss, 0.672 [0.659, 0.684], h 0.055 | loss, 0.901 [0.897, 0.904], h 0.050 | reported, 0.899 [0.895, 0.902] | reported, 0.683 [0.582, 0.790] | loss, 2.250 (1.667) **severe memory loss** | loss, 2.249 (1.666) **severe memory loss** |
| 10 (n 64, rho 4) | loss, 0.689 [0.678, 0.708], h 0.050 | loss, 0.902 [0.899, 0.908], h 0.050 | reported, 0.900 [0.897, 0.906] | reported, 0.691 [0.604, 0.789] | loss, 2.250 (1.667) **severe memory loss** | loss, 2.249 (1.666) **severe memory loss** |
| 11 (n 64, rho 16) | loss, 0.716 [0.697, 0.733], h 0.050 | loss, 0.902 [0.899, 0.939], h 0.050 | reported, 0.900 [0.897, 0.937] | reported, 0.804 [0.641, 0.897] | loss, 2.250 (1.667) **severe memory loss** | loss, 2.249 (1.666) **severe memory loss** |
| 12 (n 64, rho 64) | loss, 0.719 [0.680, 0.740], h 0.050 | loss, 0.897 [0.892, 0.904], h 0.050 | reported, 0.895 [0.890, 0.902] | reported, 0.510 [0.342, 0.743] | loss, 2.250 (1.667) **severe memory loss** | loss, 2.249 (1.666) **severe memory loss** |
| 13 (n 128, rho 1) | loss, 0.659 [0.646, 0.671], h 0.050 | loss, 0.906 [0.895, 0.914], h 0.050 | reported, 0.904 [0.894, 0.913] | reported, 0.279 [0.255, 0.319] | loss, 2.500 (1.917) **severe memory loss** | loss, 2.500 (1.917) **severe memory loss** |
| 14 (n 128, rho 4) | loss, 0.686 [0.667, 0.721], h 0.050 | loss, 0.902 [0.898, 0.909], h 0.050 | reported, 0.901 [0.897, 0.908] | reported, 0.255 [0.185, 0.303] | loss, 2.500 (1.917) **severe memory loss** | loss, 2.500 (1.917) **severe memory loss** |
| 15 (n 128, rho 16) | loss, 0.689 [0.684, 0.707], h 0.050 | loss, 0.904 [0.898, 0.912], h 0.050 | reported, 0.903 [0.897, 0.911] | reported, 0.314 [0.260, 0.374] | loss, 2.500 (1.917) **severe memory loss** | loss, 2.500 (1.917) **severe memory loss** |
| 16 (n 128, rho 64) | loss, 0.711 [0.691, 0.737], h 0.050 | loss, 0.914 [0.903, 0.923], h 0.050 | reported, 0.912 [0.901, 0.922] | reported, 0.308 [0.252, 0.379] | loss, 2.500 (1.917) **severe memory loss** | loss, 2.500 (1.917) **severe memory loss** |

### Hash-M-fixed against TC (against TC)

| job | creation: verdict, ratio [lo, hi], h | operation: verdict, ratio [lo, hi], h | total (reported) | read-out (reported) | B_used (end): verdict, ratio (after creation) | B_heap (end): verdict, ratio (after creation) |
| --- | --- | --- | --- | --- | --- | --- |
| 1 (n 16, rho 1) | loss, 0.639 [0.627, 0.655], h 0.050 | loss, 0.858 [0.850, 0.866], h 0.050 | reported, 0.850 [0.842, 0.858] | reported, 0.585 [0.576, 0.669] | loss, 2.417 (1.667) **severe memory loss** | loss, 2.483 (1.667) **severe memory loss** |
| 2 (n 16, rho 4) | loss, 0.664 [0.653, 0.675], h 0.050 | loss, 0.857 [0.853, 0.864], h 0.050 | reported, 0.850 [0.845, 0.856] | reported, 0.582 [0.570, 0.683] | loss, 2.417 (1.667) **severe memory loss** | loss, 2.483 (1.667) **severe memory loss** |
| 3 (n 16, rho 16) | loss, 0.675 [0.658, 0.702], h 0.050 | loss, 0.839 [0.836, 0.842], h 0.050 | reported, 0.833 [0.830, 0.837] | reported, 0.583 [0.573, 0.596] | loss, 2.417 (1.667) **severe memory loss** | loss, 2.483 (1.667) **severe memory loss** |
| 4 (n 16, rho 64) | loss, 0.678 [0.668, 0.685], h 0.050 | loss, 0.844 [0.840, 0.849], h 0.050 | reported, 0.837 [0.834, 0.842] | reported, 0.588 [0.577, 0.611] | loss, 2.417 (1.667) **severe memory loss** | loss, 2.483 (1.667) **severe memory loss** |
| 5 (n 32, rho 1) | loss, 0.700 [0.694, 0.711], h 0.050 | loss, 0.887 [0.883, 0.892], h 0.050 | reported, 0.883 [0.879, 0.888] | reported, 0.643 [0.604, 0.772] | loss, 2.417 (1.667) **severe memory loss** | loss, 2.413 (1.667) **severe memory loss** |
| 6 (n 32, rho 4) | loss, 0.685 [0.679, 0.696], h 0.050 | loss, 0.891 [0.885, 0.894], h 0.050 | reported, 0.887 [0.881, 0.890] | reported, 0.656 [0.637, 0.814] | loss, 2.417 (1.667) **severe memory loss** | loss, 2.413 (1.667) **severe memory loss** |
| 7 (n 32, rho 16) | loss, 0.734 [0.725, 0.745], h 0.050 | loss, 0.890 [0.883, 0.908], h 0.050 | reported, 0.887 [0.879, 0.904] | reported, 0.656 [0.616, 0.660] | loss, 2.417 (1.667) **severe memory loss** | loss, 2.413 (1.667) **severe memory loss** |
| 8 (n 32, rho 64) | loss, 0.749 [0.741, 0.762], h 0.050 | loss, 0.879 [0.877, 0.883], h 0.050 | reported, 0.877 [0.874, 0.881] | reported, 0.621 [0.605, 0.737] | loss, 2.417 (1.667) **severe memory loss** | loss, 2.413 (1.667) **severe memory loss** |
| 9 (n 64, rho 1) | loss, 0.699 [0.692, 0.712], h 0.055 | loss, 0.893 [0.888, 0.897], h 0.050 | reported, 0.891 [0.886, 0.895] | reported, 0.731 [0.550, 0.777] | loss, 2.417 (1.667) **severe memory loss** | loss, 2.416 (1.667) **severe memory loss** |
| 10 (n 64, rho 4) | loss, 0.728 [0.709, 0.737], h 0.050 | loss, 0.899 [0.895, 0.903], h 0.050 | reported, 0.897 [0.893, 0.902] | reported, 0.713 [0.560, 0.859] | loss, 2.417 (1.667) **severe memory loss** | loss, 2.416 (1.667) **severe memory loss** |
| 11 (n 64, rho 16) | loss, 0.754 [0.740, 0.772], h 0.050 | loss, 0.897 [0.894, 0.936], h 0.050 | reported, 0.896 [0.893, 0.935] | reported, 0.800 [0.621, 0.907] | loss, 2.417 (1.667) **severe memory loss** | loss, 2.416 (1.667) **severe memory loss** |
| 12 (n 64, rho 64) | loss, 0.765 [0.740, 0.782], h 0.050 | loss, 0.895 [0.889, 0.899], h 0.050 | reported, 0.893 [0.887, 0.898] | reported, 0.581 [0.510, 0.710] | loss, 2.417 (1.667) **severe memory loss** | loss, 2.416 (1.667) **severe memory loss** |
| 13 (n 128, rho 1) | loss, 0.679 [0.658, 0.703], h 0.050 | loss, 0.885 [0.867, 0.894], h 0.050 | reported, 0.884 [0.866, 0.892] | reported, 0.207 [0.188, 0.247] | loss, 2.833 (1.917) **severe memory loss** | loss, 2.833 (1.917) **severe memory loss** |
| 14 (n 128, rho 4) | loss, 0.710 [0.685, 0.740], h 0.050 | loss, 0.889 [0.879, 0.896], h 0.050 | reported, 0.888 [0.878, 0.895] | reported, 0.232 [0.189, 0.277] | loss, 2.833 (1.917) **severe memory loss** | loss, 2.833 (1.917) **severe memory loss** |
| 15 (n 128, rho 16) | loss, 0.727 [0.709, 0.743], h 0.050 | loss, 0.894 [0.890, 0.899], h 0.050 | reported, 0.892 [0.889, 0.898] | reported, 0.268 [0.215, 0.403] | loss, 2.833 (1.917) **severe memory loss** | loss, 2.833 (1.917) **severe memory loss** |
| 16 (n 128, rho 64) | loss, 0.731 [0.689, 0.756], h 0.050 | loss, 0.899 [0.892, 0.905], h 0.050 | reported, 0.898 [0.891, 0.904] | reported, 0.310 [0.206, 0.374] | loss, 2.833 (1.917) **severe memory loss** | loss, 2.833 (1.917) **severe memory loss** |

### Hash-V-grow against TC (against TC)

| job | creation: verdict, ratio [lo, hi], h | operation: verdict, ratio [lo, hi], h | total (reported) | read-out (reported) | B_used (end): verdict, ratio (after creation) | B_heap (end): verdict, ratio (after creation) |
| --- | --- | --- | --- | --- | --- | --- |
| 1 (n 16, rho 1) | loss, 0.264 [0.261, 0.271], h 0.050 | loss, 0.850 [0.845, 0.857], h 0.050 | reported, 0.800 [0.795, 0.807] | reported, 0.597 [0.588, 0.685] | loss, 2.250 (1.585) **severe memory loss** | loss, 2.405 (1.752) **severe memory loss** |
| 2 (n 16, rho 4) | loss, 0.454 [0.447, 0.460], h 0.050 | loss, 0.849 [0.847, 0.856], h 0.050 | reported, 0.828 [0.825, 0.834] | reported, 0.600 [0.588, 0.698] | loss, 1.250 (0.585) | loss, 1.427 (0.775) |
| 3 (n 16, rho 16) | loss, 0.572 [0.562, 0.588], h 0.050 | loss, 0.816 [0.810, 0.820], h 0.050 | reported, 0.807 [0.800, 0.810] | reported, 0.601 [0.591, 0.613] | tie, 1.000 (0.335) | loss, 1.183 (0.406) |
| 4 (n 16, rho 64) | loss, 0.622 [0.612, 0.632], h 0.050 | loss, 0.820 [0.816, 0.825], h 0.050 | reported, 0.813 [0.809, 0.818] | reported, 0.600 [0.589, 0.620] | tie, 0.948 (0.283) | loss, 1.121 (0.322) |
| 5 (n 32, rho 1) | loss, 0.394 [0.390, 0.398], h 0.050 | loss, 0.881 [0.877, 0.885], h 0.050 | reported, 0.864 [0.860, 0.869] | reported, 0.656 [0.612, 0.775] | loss, 1.583 (0.917) | loss, 1.627 (0.963) |
| 6 (n 32, rho 4) | loss, 0.455 [0.450, 0.463], h 0.050 | loss, 0.883 [0.879, 0.887], h 0.050 | reported, 0.871 [0.866, 0.874] | reported, 0.662 [0.632, 0.822] | loss, 1.250 (0.584) | loss, 1.295 (0.632) |
| 7 (n 32, rho 16) | loss, 0.634 [0.625, 0.644], h 0.050 | loss, 0.876 [0.870, 0.894], h 0.050 | reported, 0.871 [0.864, 0.889] | reported, 0.668 [0.620, 0.680] | tie, 1.000 (0.334) | tie, 1.047 (0.383) |
| 8 (n 32, rho 64) | loss, 0.656 [0.652, 0.665], h 0.050 | loss, 0.867 [0.859, 0.871], h 0.050 | reported, 0.863 [0.855, 0.866] | reported, 0.636 [0.607, 0.759] | tie, 0.938 (0.271) | tie, 0.984 (0.290) |
| 9 (n 64, rho 1) | loss, 0.396 [0.390, 0.403], h 0.055 | loss, 0.892 [0.885, 0.896], h 0.050 | reported, 0.883 [0.875, 0.887] | reported, 0.777 [0.675, 0.819] | loss, 1.583 (0.917) | loss, 1.594 (0.928) |
| 10 (n 64, rho 4) | loss, 0.566 [0.557, 0.573], h 0.050 | loss, 0.896 [0.891, 0.901], h 0.050 | reported, 0.891 [0.887, 0.897] | reported, 0.753 [0.649, 0.870] | tie, 1.083 (0.417) | tie, 1.095 (0.429) |
| 11 (n 64, rho 16) | loss, 0.614 [0.560, 0.635], h 0.050 | loss, 0.893 [0.882, 0.931], h 0.050 | reported, 0.890 [0.877, 0.928] | reported, 0.819 [0.665, 0.887] | tie, 1.000 (0.333) | tie, 1.012 (0.346) |
| 12 (n 64, rho 64) | loss, 0.662 [0.632, 0.680], h 0.050 | loss, 0.887 [0.880, 0.893], h 0.050 | reported, 0.884 [0.877, 0.889] | reported, 0.718 [0.616, 0.859] | tie, 0.938 (0.271) | tie, 0.949 (0.284) |
| 13 (n 128, rho 1) | loss, 0.424 [0.413, 0.431], h 0.050 | loss, 0.904 [0.896, 0.910], h 0.050 | reported, 0.900 [0.892, 0.906] | reported, 0.860 [0.769, 0.934] | loss, 1.833 (1.167) | loss, 1.836 (1.170) |
| 14 (n 128, rho 4) | loss, 0.598 [0.576, 0.623], h 0.050 | loss, 0.901 [0.893, 0.910], h 0.050 | reported, 0.899 [0.891, 0.908] | reported, 0.989 [0.825, 1.049] | loss, 1.333 (0.667) | loss, 1.336 (0.670) |
| 15 (n 128, rho 16) | loss, 0.654 [0.646, 0.669], h 0.050 | loss, 0.909 [0.885, 0.914], h 0.050 | reported, 0.908 [0.884, 0.912] | reported, 1.021 [0.748, 1.087] | loss, 1.208 (0.542) | loss, 1.211 (0.545) |
| 16 (n 128, rho 64) | loss, 0.710 [0.695, 0.726], h 0.050 | loss, 0.899 [0.873, 0.916], h 0.050 | reported, 0.898 [0.872, 0.915] | reported, 1.016 [0.854, 1.096] | loss, 1.188 (0.521) | loss, 1.190 (0.524) |

### Hash-M-grow against TC (against TC)

| job | creation: verdict, ratio [lo, hi], h | operation: verdict, ratio [lo, hi], h | total (reported) | read-out (reported) | B_used (end): verdict, ratio (after creation) | B_heap (end): verdict, ratio (after creation) |
| --- | --- | --- | --- | --- | --- | --- |
| 1 (n 16, rho 1) | loss, 0.266 [0.260, 0.275], h 0.050 | loss, 0.826 [0.821, 0.833], h 0.050 | reported, 0.780 [0.775, 0.787] | reported, 0.596 [0.587, 0.682] | loss, 2.417 (1.585) **severe memory loss** | loss, 2.573 (1.756) **severe memory loss** |
| 2 (n 16, rho 4) | loss, 0.462 [0.452, 0.471], h 0.050 | loss, 0.825 [0.819, 0.833], h 0.050 | reported, 0.807 [0.800, 0.814] | reported, 0.597 [0.583, 0.702] | loss, 1.292 (0.585) | loss, 1.473 (0.779) |
| 3 (n 16, rho 16) | loss, 0.588 [0.575, 0.606], h 0.050 | loss, 0.811 [0.802, 0.815], h 0.050 | reported, 0.802 [0.794, 0.806] | reported, 0.601 [0.579, 0.614] | tie, 1.010 (0.335) | loss, 1.187 (0.410) |
| 4 (n 16, rho 64) | loss, 0.636 [0.625, 0.643], h 0.050 | loss, 0.813 [0.811, 0.819], h 0.050 | reported, 0.807 [0.804, 0.812] | reported, 0.608 [0.597, 0.628] | tie, 0.953 (0.283) | loss, 1.131 (0.326) |
| 5 (n 32, rho 1) | loss, 0.392 [0.390, 0.398], h 0.050 | loss, 0.868 [0.866, 0.873], h 0.050 | reported, 0.852 [0.849, 0.856] | reported, 0.662 [0.628, 0.789] | loss, 1.667 (0.917) | loss, 1.711 (0.964) |
| 6 (n 32, rho 4) | loss, 0.448 [0.444, 0.455], h 0.050 | loss, 0.873 [0.869, 0.876], h 0.050 | reported, 0.860 [0.856, 0.863] | reported, 0.666 [0.642, 0.831] | loss, 1.292 (0.584) | loss, 1.338 (0.633) |
| 7 (n 32, rho 16) | loss, 0.637 [0.621, 0.643], h 0.050 | loss, 0.872 [0.866, 0.888], h 0.050 | reported, 0.867 [0.860, 0.883] | reported, 0.664 [0.628, 0.674] | tie, 1.010 (0.334) | tie, 1.058 (0.384) |
| 8 (n 32, rho 64) | loss, 0.666 [0.659, 0.673], h 0.050 | loss, 0.860 [0.856, 0.864], h 0.050 | reported, 0.856 [0.852, 0.860] | reported, 0.640 [0.614, 0.762] | tie, 0.940 (0.271) | tie, 0.985 (0.291) |
| 9 (n 64, rho 1) | loss, 0.396 [0.388, 0.402], h 0.055 | loss, 0.880 [0.873, 0.885], h 0.050 | reported, 0.872 [0.864, 0.877] | reported, 0.752 [0.646, 0.847] | loss, 1.667 (0.917) | loss, 1.678 (0.929) |
| 10 (n 64, rho 4) | loss, 0.561 [0.557, 0.567], h 0.050 | loss, 0.883 [0.879, 0.888], h 0.050 | reported, 0.879 [0.875, 0.884] | reported, 0.752 [0.620, 0.890] | loss, 1.104 (0.417) | loss, 1.116 (0.429) |
| 11 (n 64, rho 16) | loss, 0.626 [0.618, 0.639], h 0.050 | loss, 0.884 [0.881, 0.920], h 0.050 | reported, 0.881 [0.877, 0.917] | reported, 0.848 [0.678, 0.931] | tie, 1.010 (0.333) | tie, 1.022 (0.346) |
| 12 (n 64, rho 64) | loss, 0.677 [0.658, 0.694], h 0.050 | loss, 0.882 [0.874, 0.887], h 0.050 | reported, 0.880 [0.872, 0.884] | reported, 0.754 [0.694, 0.885] | tie, 0.940 (0.271) | tie, 0.952 (0.284) |
| 13 (n 128, rho 1) | loss, 0.429 [0.420, 0.437], h 0.050 | loss, 0.889 [0.881, 0.896], h 0.050 | reported, 0.885 [0.877, 0.892] | reported, 0.641 [0.590, 0.733] | loss, 2.000 (1.167) **severe memory loss** | loss, 2.003 (1.170) **severe memory loss** |
| 14 (n 128, rho 4) | loss, 0.598 [0.585, 0.620], h 0.050 | loss, 0.896 [0.884, 0.902], h 0.050 | reported, 0.895 [0.882, 0.900] | reported, 0.942 [0.864, 1.023] | loss, 1.375 (0.667) | loss, 1.378 (0.670) |
| 15 (n 128, rho 16) | loss, 0.655 [0.642, 0.667], h 0.050 | loss, 0.895 [0.890, 0.900], h 0.050 | reported, 0.894 [0.889, 0.899] | reported, 1.047 [0.913, 1.088] | loss, 1.219 (0.542) | loss, 1.222 (0.545) |
| 16 (n 128, rho 64) | loss, 0.719 [0.701, 0.734], h 0.050 | loss, 0.901 [0.888, 0.908], h 0.050 | reported, 0.900 [0.887, 0.907] | reported, 1.004 [0.863, 1.080] | loss, 1.193 (0.521) | loss, 1.196 (0.524) |

### Hash-M-fixed against Hash-V-fixed (M against V)

| job | creation: verdict, ratio [lo, hi], h | operation: verdict, ratio [lo, hi], h | total (reported) | read-out (reported) | B_used (end): verdict, ratio (after creation) | B_heap (end): verdict, ratio (after creation) |
| --- | --- | --- | --- | --- | --- | --- |
| 1 (n 16, rho 1) | win, 1.063 [1.050, 1.070], h 0.050 | tie, 0.972 [0.965, 0.976], h 0.050 | reported, 0.976 [0.969, 0.980] | reported, 0.969 [0.949, 0.982] | tie, 1.074 (1.000) | tie, 1.073 (1.002) |
| 2 (n 16, rho 4) | win, 1.081 [1.069, 1.102], h 0.050 | tie, 0.979 [0.973, 0.986], h 0.050 | reported, 0.984 [0.978, 0.990] | reported, 0.972 [0.952, 0.992] | tie, 1.074 (1.000) | tie, 1.073 (1.002) |
| 3 (n 16, rho 16) | tie, 1.067 [1.042, 1.088], h 0.050 | tie, 0.999 [0.991, 1.007], h 0.050 | reported, 1.002 [0.994, 1.010] | reported, 0.969 [0.952, 1.001] | tie, 1.074 (1.000) | tie, 1.073 (1.002) |
| 4 (n 16, rho 64) | tie, 1.034 [1.018, 1.051], h 0.050 | tie, 0.993 [0.987, 1.002], h 0.050 | reported, 0.993 [0.988, 1.003] | reported, 0.975 [0.960, 1.020] | tie, 1.074 (1.000) | tie, 1.073 (1.002) |
| 5 (n 32, rho 1) | tie, 1.053 [1.043, 1.062], h 0.050 | tie, 0.991 [0.986, 0.994], h 0.050 | reported, 0.992 [0.987, 0.995] | reported, 0.988 [0.949, 1.015] | tie, 1.074 (1.000) | tie, 1.074 (1.001) |
| 6 (n 32, rho 4) | tie, 1.042 [1.036, 1.051], h 0.050 | tie, 0.993 [0.989, 0.999], h 0.050 | reported, 0.994 [0.990, 1.000] | reported, 0.981 [0.969, 1.029] | tie, 1.074 (1.000) | tie, 1.074 (1.001) |
| 7 (n 32, rho 16) | tie, 1.055 [1.042, 1.074], h 0.050 | tie, 0.995 [0.989, 1.006], h 0.050 | reported, 0.996 [0.990, 1.006] | reported, 0.987 [0.953, 0.995] | tie, 1.074 (1.000) | tie, 1.074 (1.001) |
| 8 (n 32, rho 64) | tie, 1.051 [1.040, 1.071], h 0.050 | tie, 0.996 [0.993, 1.002], h 0.050 | reported, 0.997 [0.994, 1.003] | reported, 0.973 [0.960, 0.987] | tie, 1.074 (1.000) | tie, 1.074 (1.001) |
| 9 (n 64, rho 1) | tie, 1.040 [1.030, 1.063], h 0.055 | tie, 0.991 [0.987, 0.995], h 0.050 | reported, 0.991 [0.987, 0.996] | reported, 1.070 [0.764, 1.170] | tie, 1.074 (1.000) | tie, 1.074 (1.000) |
| 10 (n 64, rho 4) | tie, 1.057 [1.026, 1.068], h 0.050 | tie, 0.997 [0.991, 1.000], h 0.050 | reported, 0.997 [0.991, 1.001] | reported, 1.033 [0.885, 1.179] | tie, 1.074 (1.000) | tie, 1.074 (1.000) |
| 11 (n 64, rho 16) | tie, 1.053 [1.038, 1.084], h 0.050 | tie, 0.995 [0.991, 0.998], h 0.050 | reported, 0.995 [0.991, 0.998] | reported, 0.995 [0.872, 1.094] | tie, 1.074 (1.000) | tie, 1.074 (1.000) |
| 12 (n 64, rho 64) | tie, 1.065 [1.038, 1.109], h 0.050 | tie, 0.997 [0.993, 1.002], h 0.050 | reported, 0.998 [0.993, 1.003] | reported, 1.138 [0.831, 1.566] | tie, 1.074 (1.000) | tie, 1.074 (1.000) |
| 13 (n 128, rho 1) | tie, 1.031 [1.007, 1.056], h 0.050 | tie, 0.977 [0.956, 0.989], h 0.050 | reported, 0.977 [0.956, 0.989] | reported, 0.740 [0.627, 0.876] | loss, 1.133 (1.000) | loss, 1.133 (1.000) |
| 14 (n 128, rho 4) | tie, 1.035 [1.011, 1.059], h 0.050 | tie, 0.986 [0.975, 0.991], h 0.050 | reported, 0.986 [0.975, 0.991] | reported, 0.908 [0.664, 1.338] | loss, 1.133 (1.000) | loss, 1.133 (1.000) |
| 15 (n 128, rho 16) | tie, 1.056 [1.024, 1.065], h 0.050 | tie, 0.988 [0.983, 0.996], h 0.050 | reported, 0.989 [0.983, 0.996] | reported, 0.854 [0.707, 1.283] | loss, 1.133 (1.000) | loss, 1.133 (1.000) |
| 16 (n 128, rho 64) | tie, 1.027 [0.976, 1.063], h 0.050 | tie, 0.984 [0.978, 0.996], h 0.050 | reported, 0.984 [0.979, 0.996] | reported, 1.005 [0.626, 1.261] | loss, 1.133 (1.000) | loss, 1.133 (1.000) |

### Hash-M-grow against Hash-V-grow (M against V)

| job | creation: verdict, ratio [lo, hi], h | operation: verdict, ratio [lo, hi], h | total (reported) | read-out (reported) | B_used (end): verdict, ratio (after creation) | B_heap (end): verdict, ratio (after creation) |
| --- | --- | --- | --- | --- | --- | --- |
| 1 (n 16, rho 1) | tie, 1.008 [0.993, 1.021], h 0.050 | tie, 0.972 [0.967, 0.978], h 0.050 | reported, 0.975 [0.969, 0.981] | reported, 0.998 [0.988, 1.008] | tie, 1.074 (1.000) | tie, 1.070 (1.002) |
| 2 (n 16, rho 4) | tie, 1.016 [0.999, 1.036], h 0.050 | tie, 0.971 [0.963, 0.976], h 0.050 | reported, 0.975 [0.965, 0.979] | reported, 0.997 [0.979, 1.017] | tie, 1.033 (1.000) | tie, 1.032 (1.005) |
| 3 (n 16, rho 16) | tie, 1.028 [1.011, 1.045], h 0.050 | tie, 0.994 [0.980, 1.002], h 0.050 | reported, 0.994 [0.981, 1.004] | reported, 1.000 [0.965, 1.023] | tie, 1.010 (1.000) | tie, 1.003 (1.009) |
| 4 (n 16, rho 64) | tie, 1.022 [1.006, 1.034], h 0.050 | tie, 0.992 [0.990, 0.997], h 0.050 | reported, 0.993 [0.991, 0.998] | reported, 1.012 [0.997, 1.045] | tie, 1.005 (1.000) | tie, 1.009 (1.012) |
| 5 (n 32, rho 1) | tie, 0.995 [0.992, 1.007], h 0.050 | tie, 0.985 [0.982, 0.989], h 0.050 | reported, 0.986 [0.983, 0.989] | reported, 1.009 [0.998, 1.059] | tie, 1.053 (1.000) | tie, 1.052 (1.001) |
| 6 (n 32, rho 4) | tie, 0.985 [0.976, 0.996], h 0.050 | tie, 0.989 [0.986, 0.992], h 0.050 | reported, 0.988 [0.985, 0.992] | reported, 1.006 [0.983, 1.031] | tie, 1.033 (1.000) | tie, 1.033 (1.002) |
| 7 (n 32, rho 16) | tie, 1.005 [0.985, 1.017], h 0.050 | tie, 0.995 [0.988, 1.003], h 0.050 | reported, 0.995 [0.988, 1.003] | reported, 0.994 [0.975, 1.043] | tie, 1.010 (1.000) | tie, 1.011 (1.003) |
| 8 (n 32, rho 64) | tie, 1.014 [0.999, 1.026], h 0.050 | tie, 0.992 [0.986, 1.000], h 0.050 | reported, 0.992 [0.987, 1.001] | reported, 1.006 [0.974, 1.033] | tie, 1.003 (1.000) | tie, 1.001 (1.003) |
| 9 (n 64, rho 1) | tie, 1.000 [0.975, 1.010], h 0.055 | tie, 0.987 [0.979, 0.995], h 0.050 | reported, 0.987 [0.979, 0.996] | reported, 0.968 [0.894, 1.076] | tie, 1.053 (1.000) | tie, 1.052 (1.000) |
| 10 (n 64, rho 4) | tie, 0.991 [0.983, 1.009], h 0.050 | tie, 0.986 [0.983, 0.989], h 0.050 | reported, 0.986 [0.983, 0.989] | reported, 0.999 [0.946, 1.035] | tie, 1.019 (1.000) | tie, 1.019 (1.001) |
| 11 (n 64, rho 16) | tie, 1.019 [1.000, 1.122], h 0.050 | tie, 0.990 [0.988, 1.000], h 0.050 | reported, 0.991 [0.989, 1.002] | reported, 1.036 [0.997, 1.075] | tie, 1.010 (1.000) | tie, 1.011 (1.001) |
| 12 (n 64, rho 64) | tie, 1.023 [0.999, 1.064], h 0.050 | tie, 0.995 [0.990, 1.002], h 0.050 | reported, 0.995 [0.990, 1.003] | reported, 1.050 [0.966, 1.185] | tie, 1.003 (1.000) | tie, 1.003 (1.001) |
| 13 (n 128, rho 1) | tie, 1.011 [1.000, 1.038], h 0.050 | tie, 0.983 [0.976, 0.992], h 0.050 | reported, 0.983 [0.976, 0.992] | reported, 0.746 [0.683, 0.888] | tie, 1.091 (1.000) | tie, 1.091 (1.000) |
| 14 (n 128, rho 4) | tie, 1.001 [0.987, 1.033], h 0.050 | tie, 0.994 [0.986, 1.000], h 0.050 | reported, 0.995 [0.986, 1.000] | reported, 0.952 [0.900, 1.115] | tie, 1.031 (1.000) | tie, 1.031 (1.000) |
| 15 (n 128, rho 16) | tie, 1.002 [0.979, 1.014], h 0.050 | tie, 0.984 [0.979, 1.011], h 0.050 | reported, 0.985 [0.979, 1.011] | reported, 1.025 [0.957, 1.324] | tie, 1.009 (1.000) | tie, 1.009 (1.000) |
| 16 (n 128, rho 64) | tie, 1.012 [0.999, 1.023], h 0.050 | tie, 1.003 [0.981, 1.033], h 0.050 | reported, 1.003 [0.981, 1.033] | reported, 0.987 [0.843, 1.147] | tie, 1.004 (1.000) | tie, 1.004 (1.000) |

### Hash-V-grow against Hash-V-fixed (grow against fixed)

| job | creation: verdict, ratio [lo, hi], h | operation: verdict, ratio [lo, hi], h | total (reported) | read-out (reported) | B_used (end): verdict, ratio (after creation) | B_heap (end): verdict, ratio (after creation) |
| --- | --- | --- | --- | --- | --- | --- |
| 1 (n 16, rho 1) | loss, 0.439 [0.436, 0.443], h 0.050 | tie, 0.962 [0.957, 0.968], h 0.050 | reported, 0.918 [0.913, 0.924] | reported, 0.989 [0.983, 1.003] | tie, 1.000 (0.951) | tie, 1.038 (1.054) |
| 2 (n 16, rho 4) | loss, 0.740 [0.733, 0.748], h 0.050 | tie, 0.970 [0.967, 0.976], h 0.050 | reported, 0.959 [0.955, 0.964] | reported, 1.001 [0.980, 1.021] | win, 0.556 (0.351) | win, 0.616 (0.466) |
| 3 (n 16, rho 16) | loss, 0.904 [0.887, 0.918], h 0.050 | tie, 0.972 [0.964, 0.982], h 0.050 | reported, 0.970 [0.961, 0.979] | reported, 0.999 [0.972, 1.032] | win, 0.444 (0.201) | win, 0.511 (0.244) |
| 4 (n 16, rho 64) | tie, 0.949 [0.939, 0.962], h 0.050 | tie, 0.965 [0.959, 0.972], h 0.050 | reported, 0.964 [0.958, 0.972] | reported, 0.994 [0.952, 1.039] | win, 0.421 (0.170) | win, 0.484 (0.194) |
| 5 (n 32, rho 1) | loss, 0.592 [0.587, 0.595], h 0.050 | tie, 0.984 [0.978, 0.987], h 0.050 | reported, 0.970 [0.965, 0.973] | reported, 1.008 [0.961, 1.020] | win, 0.704 (0.550) | win, 0.724 (0.578) |
| 6 (n 32, rho 4) | loss, 0.693 [0.687, 0.697], h 0.050 | tie, 0.985 [0.981, 0.990], h 0.050 | reported, 0.976 [0.973, 0.981] | reported, 0.990 [0.964, 1.037] | win, 0.556 (0.350) | win, 0.577 (0.379) |
| 7 (n 32, rho 16) | loss, 0.911 [0.896, 0.926], h 0.050 | tie, 0.980 [0.973, 0.993], h 0.050 | reported, 0.978 [0.971, 0.991] | reported, 1.005 [0.956, 1.014] | win, 0.444 (0.200) | win, 0.466 (0.230) |
| 8 (n 32, rho 64) | loss, 0.921 [0.911, 0.932], h 0.050 | tie, 0.983 [0.973, 0.987], h 0.050 | reported, 0.981 [0.972, 0.986] | reported, 0.996 [0.966, 1.012] | win, 0.417 (0.163) | win, 0.438 (0.174) |
| 9 (n 64, rho 1) | loss, 0.590 [0.581, 0.602], h 0.055 | tie, 0.990 [0.983, 0.994], h 0.050 | reported, 0.983 [0.975, 0.987] | reported, 1.138 [0.984, 1.245] | win, 0.704 (0.550) | win, 0.709 (0.557) |
| 10 (n 64, rho 4) | loss, 0.821 [0.803, 0.830], h 0.050 | tie, 0.993 [0.986, 0.997], h 0.050 | reported, 0.990 [0.984, 0.995] | reported, 1.091 [1.029, 1.169] | win, 0.481 (0.250) | win, 0.487 (0.258) |
| 11 (n 64, rho 16) | loss, 0.858 [0.775, 0.899], h 0.050 | tie, 0.990 [0.977, 0.993], h 0.050 | reported, 0.988 [0.973, 0.991] | reported, 1.018 [0.934, 1.126] | win, 0.444 (0.200) | win, 0.450 (0.208) |
| 12 (n 64, rho 64) | tie, 0.921 [0.872, 0.965], h 0.050 | tie, 0.988 [0.980, 0.993], h 0.050 | reported, 0.988 [0.980, 0.992] | reported, 1.406 [1.053, 1.917] | win, 0.417 (0.163) | win, 0.422 (0.170) |
| 13 (n 128, rho 1) | loss, 0.644 [0.630, 0.649], h 0.050 | tie, 0.998 [0.990, 1.009], h 0.050 | reported, 0.995 [0.987, 1.006] | reported, 3.079 [2.553, 3.446] | win, 0.733 (0.609) | win, 0.734 (0.610) |
| 14 (n 128, rho 4) | loss, 0.872 [0.839, 0.887], h 0.050 | tie, 0.999 [0.992, 1.007], h 0.050 | reported, 0.999 [0.992, 1.007] | reported, 3.872 [3.104, 5.376] | win, 0.533 (0.348) | win, 0.535 (0.349) |
| 15 (n 128, rho 16) | tie, 0.949 [0.928, 0.956], h 0.050 | tie, 1.006 [0.979, 1.010], h 0.050 | reported, 1.006 [0.978, 1.010] | reported, 3.249 [2.330, 3.803] | win, 0.483 (0.283) | win, 0.485 (0.284) |
| 16 (n 128, rho 64) | tie, 0.999 [0.975, 1.023], h 0.050 | tie, 0.984 [0.955, 1.002], h 0.050 | reported, 0.984 [0.955, 1.003] | reported, 3.296 [2.566, 4.019] | win, 0.475 (0.272) | win, 0.476 (0.273) |

### Hash-M-grow against Hash-M-fixed (grow against fixed)

| job | creation: verdict, ratio [lo, hi], h | operation: verdict, ratio [lo, hi], h | total (reported) | read-out (reported) | B_used (end): verdict, ratio (after creation) | B_heap (end): verdict, ratio (after creation) |
| --- | --- | --- | --- | --- | --- | --- |
| 1 (n 16, rho 1) | loss, 0.417 [0.411, 0.423], h 0.050 | tie, 0.962 [0.957, 0.968], h 0.050 | reported, 0.917 [0.913, 0.924] | reported, 1.020 [1.013, 1.038] | tie, 1.000 (0.951) | tie, 1.036 (1.053) |
| 2 (n 16, rho 4) | loss, 0.695 [0.679, 0.706], h 0.050 | tie, 0.963 [0.956, 0.967], h 0.050 | reported, 0.950 [0.942, 0.954] | reported, 1.026 [1.007, 1.046] | win, 0.534 (0.351) | win, 0.593 (0.467) |
| 3 (n 16, rho 16) | loss, 0.871 [0.855, 0.884], h 0.050 | tie, 0.967 [0.953, 0.973], h 0.050 | reported, 0.962 [0.950, 0.969] | reported, 1.030 [0.995, 1.044] | win, 0.418 (0.201) | win, 0.478 (0.246) |
| 4 (n 16, rho 64) | loss, 0.938 [0.923, 0.947], h 0.050 | tie, 0.964 [0.961, 0.970], h 0.050 | reported, 0.965 [0.961, 0.970] | reported, 1.032 [1.017, 1.041] | win, 0.394 (0.170) | win, 0.455 (0.195) |
| 5 (n 32, rho 1) | loss, 0.559 [0.555, 0.564], h 0.050 | tie, 0.978 [0.974, 0.983], h 0.050 | reported, 0.964 [0.960, 0.968] | reported, 1.029 [1.003, 1.061] | win, 0.690 (0.550) | win, 0.709 (0.579) |
| 6 (n 32, rho 4) | loss, 0.655 [0.645, 0.658], h 0.050 | tie, 0.980 [0.976, 0.984], h 0.050 | reported, 0.969 [0.966, 0.974] | reported, 1.015 [0.992, 1.031] | win, 0.534 (0.350) | win, 0.555 (0.380) |
| 7 (n 32, rho 16) | loss, 0.868 [0.845, 0.879], h 0.050 | tie, 0.979 [0.974, 0.985], h 0.050 | reported, 0.978 [0.972, 0.983] | reported, 1.012 [1.006, 1.036] | win, 0.418 (0.200) | win, 0.439 (0.231) |
| 8 (n 32, rho 64) | loss, 0.888 [0.870, 0.898], h 0.050 | tie, 0.978 [0.972, 0.982], h 0.050 | reported, 0.976 [0.970, 0.980] | reported, 1.030 [0.993, 1.046] | win, 0.389 (0.163) | win, 0.408 (0.174) |
| 9 (n 64, rho 1) | loss, 0.567 [0.551, 0.573], h 0.055 | tie, 0.986 [0.978, 0.991], h 0.050 | reported, 0.979 [0.970, 0.983] | reported, 1.029 [0.950, 1.423] | win, 0.690 (0.550) | win, 0.695 (0.557) |
| 10 (n 64, rho 4) | loss, 0.770 [0.765, 0.791], h 0.050 | tie, 0.982 [0.980, 0.987], h 0.050 | reported, 0.979 [0.978, 0.985] | reported, 1.055 [0.973, 1.196] | win, 0.457 (0.250) | win, 0.462 (0.258) |
| 11 (n 64, rho 16) | loss, 0.830 [0.822, 0.845], h 0.050 | tie, 0.985 [0.982, 0.987], h 0.050 | reported, 0.984 [0.980, 0.986] | reported, 1.060 [0.978, 1.210] | win, 0.418 (0.200) | win, 0.423 (0.208) |
| 12 (n 64, rho 64) | loss, 0.885 [0.860, 0.906], h 0.050 | tie, 0.986 [0.980, 0.989], h 0.050 | reported, 0.985 [0.979, 0.988] | reported, 1.298 [1.113, 1.433] | win, 0.389 (0.163) | win, 0.394 (0.170) |
| 13 (n 128, rho 1) | loss, 0.632 [0.618, 0.645], h 0.050 | tie, 1.004 [0.995, 1.023], h 0.050 | reported, 1.001 [0.993, 1.020] | reported, 3.102 [2.602, 3.482] | win, 0.706 (0.609) | win, 0.707 (0.610) |
| 14 (n 128, rho 4) | loss, 0.843 [0.828, 0.859], h 0.050 | tie, 1.008 [0.997, 1.018], h 0.050 | reported, 1.007 [0.996, 1.018] | reported, 4.060 [3.379, 5.080] | win, 0.485 (0.348) | win, 0.486 (0.349) |
| 15 (n 128, rho 16) | loss, 0.901 [0.879, 0.923], h 0.050 | tie, 1.002 [0.995, 1.005], h 0.050 | reported, 1.002 [0.995, 1.005] | reported, 3.900 [2.631, 4.721] | win, 0.430 (0.283) | win, 0.431 (0.284) |
| 16 (n 128, rho 64) | tie, 0.984 [0.962, 1.033], h 0.050 | tie, 1.003 [0.990, 1.011], h 0.050 | reported, 1.003 [0.990, 1.011] | reported, 3.240 [2.408, 5.099] | win, 0.421 (0.272) | win, 0.422 (0.273) |

### Hash-V-grow-map against Hash-V-grow (map against lookup)

| job | creation: verdict, ratio [lo, hi], h | operation: verdict, ratio [lo, hi], h | total (reported) | read-out (reported) | B_used (end): verdict, ratio (after creation) | B_heap (end): verdict, ratio (after creation) |
| --- | --- | --- | --- | --- | --- | --- |
| 1 (n 16, rho 1) | win, 1.279 [1.257, 1.288], h 0.050 | reported, 1.004 [0.998, 1.009] | reported, 1.022 [1.017, 1.028] | reported, 0.903 [0.886, 0.924] | reported, 1.000 (1.000) | reported, 1.004 (1.006) |
| 2 (n 16, rho 4) | win, 1.114 [1.108, 1.123], h 0.050 | reported, 1.000 [0.995, 1.002] | reported, 1.006 [1.000, 1.008] | reported, 0.914 [0.883, 0.936] | reported, 1.000 (1.000) | reported, 1.007 (1.013) |
| 3 (n 16, rho 16) | tie, 1.033 [1.015, 1.053], h 0.050 | reported, 1.009 [1.000, 1.019] | reported, 1.009 [1.001, 1.020] | reported, 0.922 [0.898, 0.938] | reported, 1.000 (1.000) | reported, 1.009 (1.025) |
| 4 (n 16, rho 64) | tie, 1.014 [0.997, 1.025], h 0.050 | reported, 1.011 [1.004, 1.016] | reported, 1.011 [1.004, 1.016] | reported, 0.933 [0.918, 0.976] | reported, 1.000 (1.000) | reported, 1.009 (1.012) |
| 5 (n 32, rho 1) | win, 1.157 [1.151, 1.167], h 0.050 | reported, 1.005 [1.003, 1.010] | reported, 1.010 [1.007, 1.015] | reported, 0.952 [0.932, 1.002] | reported, 1.000 (1.000) | reported, 1.002 (1.003) |
| 6 (n 32, rho 4) | win, 1.156 [1.142, 1.168], h 0.050 | reported, 1.004 [1.000, 1.008] | reported, 1.008 [1.004, 1.012] | reported, 0.987 [0.968, 1.014] | reported, 1.000 (1.000) | reported, 1.002 (1.004) |
| 7 (n 32, rho 16) | tie, 1.015 [1.000, 1.024], h 0.050 | reported, 1.011 [1.001, 1.018] | reported, 1.011 [1.002, 1.018] | reported, 0.964 [0.944, 1.017] | reported, 1.000 (1.000) | reported, 1.002 (1.007) |
| 8 (n 32, rho 64) | tie, 1.005 [0.993, 1.016], h 0.050 | reported, 1.005 [1.001, 1.014] | reported, 1.005 [1.001, 1.014] | reported, 0.968 [0.933, 0.989] | reported, 1.000 (1.000) | reported, 1.003 (1.009) |
| 9 (n 64, rho 1) | win, 1.186 [1.166, 1.198], h 0.055 | reported, 1.000 [0.989, 1.006] | reported, 1.003 [0.992, 1.010] | reported, 0.976 [0.935, 1.049] | reported, 1.000 (1.000) | reported, 1.000 (1.001) |
| 10 (n 64, rho 4) | tie, 1.060 [1.047, 1.077], h 0.050 | reported, 1.001 [0.999, 1.005] | reported, 1.002 [1.000, 1.005] | reported, 0.980 [0.919, 1.019] | reported, 1.000 (1.000) | reported, 1.001 (1.002) |
| 11 (n 64, rho 16) | tie, 1.063 [1.034, 1.181], h 0.050 | reported, 1.002 [1.000, 1.010] | reported, 1.003 [1.000, 1.013] | reported, 0.977 [0.925, 1.051] | reported, 1.000 (1.000) | reported, 1.001 (1.002) |
| 12 (n 64, rho 64) | tie, 1.025 [0.987, 1.067], h 0.050 | reported, 1.007 [1.003, 1.014] | reported, 1.007 [1.003, 1.014] | reported, 1.030 [0.899, 1.143] | reported, 1.000 (1.000) | reported, 1.001 (1.002) |
| 13 (n 128, rho 1) | win, 1.156 [1.144, 1.187], h 0.050 | reported, 0.998 [0.991, 1.007] | reported, 1.000 [0.992, 1.009] | reported, 0.906 [0.816, 1.080] | reported, 1.000 (1.000) | reported, 1.000 (1.000) |
| 14 (n 128, rho 4) | tie, 1.041 [1.016, 1.075], h 0.050 | reported, 1.002 [0.992, 1.012] | reported, 1.002 [0.991, 1.013] | reported, 0.911 [0.678, 0.982] | reported, 1.000 (1.000) | reported, 1.000 (1.000) |
| 15 (n 128, rho 16) | tie, 1.009 [1.003, 1.019], h 0.050 | reported, 0.997 [0.987, 1.024] | reported, 0.997 [0.987, 1.024] | reported, 0.954 [0.874, 1.238] | reported, 1.000 (1.000) | reported, 1.000 (1.000) |
| 16 (n 128, rho 64) | tie, 1.017 [1.003, 1.025], h 0.050 | reported, 1.008 [0.989, 1.037] | reported, 1.008 [0.989, 1.038] | reported, 0.816 [0.765, 0.948] | reported, 1.000 (1.000) | reported, 1.000 (1.000) |

### Gcd work (a check, not a verdict) and the call-for-call checks

G(row)/G(TC) MUST be exactly 1 for every hash row (rule-m1 item 7); the counting runs' per-operation records of every hash row equal TC's, and V against M, grow against fixed and map against lookup take the same cases, core counts and store splits (spec section 10 items 3-6).

| job | G per operation | G(Hash-V-fixed)/G(TC), call for call | G(Hash-M-fixed)/G(TC), call for call | G(Hash-V-grow)/G(TC), call for call | G(Hash-M-grow)/G(TC), call for call | G(Hash-V-grow-map)/G(TC), call for call | M vs V (fixed, grow) | grow vs fixed (V, M) | map vs lookup |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | 34.35 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass, pass | pass, pass | pass |
| 2 | 34.05 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass, pass | pass, pass | pass |
| 3 | 30.12 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass, pass | pass, pass | pass |
| 4 | 31.76 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass, pass | pass, pass | pass |
| 5 | 35.64 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass, pass | pass, pass | pass |
| 6 | 35.31 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass, pass | pass, pass | pass |
| 7 | 35.52 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass, pass | pass, pass | pass |
| 8 | 35.46 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass, pass | pass, pass | pass |
| 9 | 37.33 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass, pass | pass, pass | pass |
| 10 | 36.99 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass, pass | pass, pass | pass |
| 11 | 37.11 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass, pass | pass, pass | pass |
| 12 | 35.88 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass, pass | pass, pass | pass |
| 13 | 38.52 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass, pass | pass, pass | pass |
| 14 | 38.28 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass, pass | pass, pass | pass |
| 15 | 38.83 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass, pass | pass, pass | pass |
| 16 | 38.43 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass, pass | pass, pass | pass |

### Strata (counts of verdicts; no measure combined, no stratum scored)

The strata are the sharing levels of A and B (rule-m1 item 11): NONE (rho = 1), LOW (rho = 4), MID (rho = 16), HIGH (rho = 64).

| stratum | pairing | measure | wins | ties | losses | over cap | n/a | reported | median ratio | severe flags |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| NONE | Hash-V-fixed vs TC | creation | 0 | 0 | 4 | 0 | 0 | 0 | 0.662 | 0 |
| NONE | Hash-V-fixed vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.898 | 0 |
| NONE | Hash-V-fixed vs TC | B_used | 0 | 0 | 4 | 0 | 0 | 0 | 2.250 | 4 |
| NONE | Hash-V-fixed vs TC | B_heap | 0 | 0 | 4 | 0 | 0 | 0 | 2.282 | 4 |
| NONE | Hash-M-fixed vs TC | creation | 0 | 0 | 4 | 0 | 0 | 0 | 0.689 | 0 |
| NONE | Hash-M-fixed vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.886 | 0 |
| NONE | Hash-M-fixed vs TC | B_used | 0 | 0 | 4 | 0 | 0 | 0 | 2.417 | 4 |
| NONE | Hash-M-fixed vs TC | B_heap | 0 | 0 | 4 | 0 | 0 | 0 | 2.450 | 4 |
| NONE | Hash-V-grow vs TC | creation | 0 | 0 | 4 | 0 | 0 | 0 | 0.395 | 0 |
| NONE | Hash-V-grow vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.887 | 0 |
| NONE | Hash-V-grow vs TC | B_used | 0 | 0 | 4 | 0 | 0 | 0 | 1.708 | 1 |
| NONE | Hash-V-grow vs TC | B_heap | 0 | 0 | 4 | 0 | 0 | 0 | 1.731 | 1 |
| NONE | Hash-M-grow vs TC | creation | 0 | 0 | 4 | 0 | 0 | 0 | 0.394 | 0 |
| NONE | Hash-M-grow vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.874 | 0 |
| NONE | Hash-M-grow vs TC | B_used | 0 | 0 | 4 | 0 | 0 | 0 | 1.833 | 2 |
| NONE | Hash-M-grow vs TC | B_heap | 0 | 0 | 4 | 0 | 0 | 0 | 1.857 | 2 |
| NONE | Hash-M-fixed vs Hash-V-fixed | creation | 1 | 3 | 0 | 0 | 0 | 0 | 1.047 | 0 |
| NONE | Hash-M-fixed vs Hash-V-fixed | operation | 0 | 4 | 0 | 0 | 0 | 0 | 0.984 | 0 |
| NONE | Hash-M-fixed vs Hash-V-fixed | B_used | 0 | 3 | 1 | 0 | 0 | 0 | 1.074 | 0 |
| NONE | Hash-M-fixed vs Hash-V-fixed | B_heap | 0 | 3 | 1 | 0 | 0 | 0 | 1.074 | 0 |
| NONE | Hash-M-grow vs Hash-V-grow | creation | 0 | 4 | 0 | 0 | 0 | 0 | 1.004 | 0 |
| NONE | Hash-M-grow vs Hash-V-grow | operation | 0 | 4 | 0 | 0 | 0 | 0 | 0.984 | 0 |
| NONE | Hash-M-grow vs Hash-V-grow | B_used | 0 | 4 | 0 | 0 | 0 | 0 | 1.063 | 0 |
| NONE | Hash-M-grow vs Hash-V-grow | B_heap | 0 | 4 | 0 | 0 | 0 | 0 | 1.061 | 0 |
| NONE | Hash-V-grow vs Hash-V-fixed | creation | 0 | 0 | 4 | 0 | 0 | 0 | 0.591 | 0 |
| NONE | Hash-V-grow vs Hash-V-fixed | operation | 0 | 4 | 0 | 0 | 0 | 0 | 0.987 | 0 |
| NONE | Hash-V-grow vs Hash-V-fixed | B_used | 3 | 1 | 0 | 0 | 0 | 0 | 0.719 | 0 |
| NONE | Hash-V-grow vs Hash-V-fixed | B_heap | 3 | 1 | 0 | 0 | 0 | 0 | 0.729 | 0 |
| NONE | Hash-M-grow vs Hash-M-fixed | creation | 0 | 0 | 4 | 0 | 0 | 0 | 0.563 | 0 |
| NONE | Hash-M-grow vs Hash-M-fixed | operation | 0 | 4 | 0 | 0 | 0 | 0 | 0.982 | 0 |
| NONE | Hash-M-grow vs Hash-M-fixed | B_used | 3 | 1 | 0 | 0 | 0 | 0 | 0.698 | 0 |
| NONE | Hash-M-grow vs Hash-M-fixed | B_heap | 3 | 1 | 0 | 0 | 0 | 0 | 0.708 | 0 |
| NONE | Hash-V-grow-map vs Hash-V-grow | creation | 4 | 0 | 0 | 0 | 0 | 0 | 1.171 | 0 |
| NONE | Hash-V-grow-map vs Hash-V-grow | operation | 0 | 0 | 0 | 0 | 0 | 4 | 1.002 | 0 |
| NONE | Hash-V-grow-map vs Hash-V-grow | B_used | 0 | 0 | 0 | 0 | 0 | 4 | 1.000 | 0 |
| NONE | Hash-V-grow-map vs Hash-V-grow | B_heap | 0 | 0 | 0 | 0 | 0 | 4 | 1.001 | 0 |
| LOW | Hash-V-fixed vs TC | creation | 0 | 0 | 4 | 0 | 0 | 0 | 0.671 | 0 |
| LOW | Hash-V-fixed vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.899 | 0 |
| LOW | Hash-V-fixed vs TC | B_used | 0 | 0 | 4 | 0 | 0 | 0 | 2.250 | 4 |
| LOW | Hash-V-fixed vs TC | B_heap | 0 | 0 | 4 | 0 | 0 | 0 | 2.282 | 4 |
| LOW | Hash-M-fixed vs TC | creation | 0 | 0 | 4 | 0 | 0 | 0 | 0.697 | 0 |
| LOW | Hash-M-fixed vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.890 | 0 |
| LOW | Hash-M-fixed vs TC | B_used | 0 | 0 | 4 | 0 | 0 | 0 | 2.417 | 4 |
| LOW | Hash-M-fixed vs TC | B_heap | 0 | 0 | 4 | 0 | 0 | 0 | 2.450 | 4 |
| LOW | Hash-V-grow vs TC | creation | 0 | 0 | 4 | 0 | 0 | 0 | 0.510 | 0 |
| LOW | Hash-V-grow vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.889 | 0 |
| LOW | Hash-V-grow vs TC | B_used | 0 | 1 | 3 | 0 | 0 | 0 | 1.250 | 0 |
| LOW | Hash-V-grow vs TC | B_heap | 0 | 1 | 3 | 0 | 0 | 0 | 1.316 | 0 |
| LOW | Hash-M-grow vs TC | creation | 0 | 0 | 4 | 0 | 0 | 0 | 0.511 | 0 |
| LOW | Hash-M-grow vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.878 | 0 |
| LOW | Hash-M-grow vs TC | B_used | 0 | 0 | 4 | 0 | 0 | 0 | 1.292 | 0 |
| LOW | Hash-M-grow vs TC | B_heap | 0 | 0 | 4 | 0 | 0 | 0 | 1.358 | 0 |
| LOW | Hash-M-fixed vs Hash-V-fixed | creation | 1 | 3 | 0 | 0 | 0 | 0 | 1.050 | 0 |
| LOW | Hash-M-fixed vs Hash-V-fixed | operation | 0 | 4 | 0 | 0 | 0 | 0 | 0.990 | 0 |
| LOW | Hash-M-fixed vs Hash-V-fixed | B_used | 0 | 3 | 1 | 0 | 0 | 0 | 1.074 | 0 |
| LOW | Hash-M-fixed vs Hash-V-fixed | B_heap | 0 | 3 | 1 | 0 | 0 | 0 | 1.074 | 0 |
| LOW | Hash-M-grow vs Hash-V-grow | creation | 0 | 4 | 0 | 0 | 0 | 0 | 0.996 | 0 |
| LOW | Hash-M-grow vs Hash-V-grow | operation | 0 | 4 | 0 | 0 | 0 | 0 | 0.987 | 0 |
| LOW | Hash-M-grow vs Hash-V-grow | B_used | 0 | 4 | 0 | 0 | 0 | 0 | 1.032 | 0 |
| LOW | Hash-M-grow vs Hash-V-grow | B_heap | 0 | 4 | 0 | 0 | 0 | 0 | 1.032 | 0 |
| LOW | Hash-V-grow vs Hash-V-fixed | creation | 0 | 0 | 4 | 0 | 0 | 0 | 0.781 | 0 |
| LOW | Hash-V-grow vs Hash-V-fixed | operation | 0 | 4 | 0 | 0 | 0 | 0 | 0.989 | 0 |
| LOW | Hash-V-grow vs Hash-V-fixed | B_used | 4 | 0 | 0 | 0 | 0 | 0 | 0.544 | 0 |
| LOW | Hash-V-grow vs Hash-V-fixed | B_heap | 4 | 0 | 0 | 0 | 0 | 0 | 0.556 | 0 |
| LOW | Hash-M-grow vs Hash-M-fixed | creation | 0 | 0 | 4 | 0 | 0 | 0 | 0.733 | 0 |
| LOW | Hash-M-grow vs Hash-M-fixed | operation | 0 | 4 | 0 | 0 | 0 | 0 | 0.981 | 0 |
| LOW | Hash-M-grow vs Hash-M-fixed | B_used | 4 | 0 | 0 | 0 | 0 | 0 | 0.510 | 0 |
| LOW | Hash-M-grow vs Hash-M-fixed | B_heap | 4 | 0 | 0 | 0 | 0 | 0 | 0.520 | 0 |
| LOW | Hash-V-grow-map vs Hash-V-grow | creation | 2 | 2 | 0 | 0 | 0 | 0 | 1.087 | 0 |
| LOW | Hash-V-grow-map vs Hash-V-grow | operation | 0 | 0 | 0 | 0 | 0 | 4 | 1.002 | 0 |
| LOW | Hash-V-grow-map vs Hash-V-grow | B_used | 0 | 0 | 0 | 0 | 0 | 4 | 1.000 | 0 |
| LOW | Hash-V-grow-map vs Hash-V-grow | B_heap | 0 | 0 | 0 | 0 | 0 | 4 | 1.001 | 0 |
| MID | Hash-V-fixed vs TC | creation | 0 | 0 | 4 | 0 | 0 | 0 | 0.692 | 0 |
| MID | Hash-V-fixed vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.898 | 0 |
| MID | Hash-V-fixed vs TC | B_used | 0 | 0 | 4 | 0 | 0 | 0 | 2.250 | 4 |
| MID | Hash-V-fixed vs TC | B_heap | 0 | 0 | 4 | 0 | 0 | 0 | 2.282 | 4 |
| MID | Hash-M-fixed vs TC | creation | 0 | 0 | 4 | 0 | 0 | 0 | 0.731 | 0 |
| MID | Hash-M-fixed vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.892 | 0 |
| MID | Hash-M-fixed vs TC | B_used | 0 | 0 | 4 | 0 | 0 | 0 | 2.417 | 4 |
| MID | Hash-M-fixed vs TC | B_heap | 0 | 0 | 4 | 0 | 0 | 0 | 2.450 | 4 |
| MID | Hash-V-grow vs TC | creation | 0 | 0 | 4 | 0 | 0 | 0 | 0.624 | 0 |
| MID | Hash-V-grow vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.884 | 0 |
| MID | Hash-V-grow vs TC | B_used | 0 | 3 | 1 | 0 | 0 | 0 | 1.000 | 0 |
| MID | Hash-V-grow vs TC | B_heap | 0 | 2 | 2 | 0 | 0 | 0 | 1.115 | 0 |
| MID | Hash-M-grow vs TC | creation | 0 | 0 | 4 | 0 | 0 | 0 | 0.631 | 0 |
| MID | Hash-M-grow vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.878 | 0 |
| MID | Hash-M-grow vs TC | B_used | 0 | 3 | 1 | 0 | 0 | 0 | 1.010 | 0 |
| MID | Hash-M-grow vs TC | B_heap | 0 | 2 | 2 | 0 | 0 | 0 | 1.123 | 0 |
| MID | Hash-M-fixed vs Hash-V-fixed | creation | 0 | 4 | 0 | 0 | 0 | 0 | 1.055 | 0 |
| MID | Hash-M-fixed vs Hash-V-fixed | operation | 0 | 4 | 0 | 0 | 0 | 0 | 0.995 | 0 |
| MID | Hash-M-fixed vs Hash-V-fixed | B_used | 0 | 3 | 1 | 0 | 0 | 0 | 1.074 | 0 |
| MID | Hash-M-fixed vs Hash-V-fixed | B_heap | 0 | 3 | 1 | 0 | 0 | 0 | 1.074 | 0 |
| MID | Hash-M-grow vs Hash-V-grow | creation | 0 | 4 | 0 | 0 | 0 | 0 | 1.012 | 0 |
| MID | Hash-M-grow vs Hash-V-grow | operation | 0 | 4 | 0 | 0 | 0 | 0 | 0.992 | 0 |
| MID | Hash-M-grow vs Hash-V-grow | B_used | 0 | 4 | 0 | 0 | 0 | 0 | 1.010 | 0 |
| MID | Hash-M-grow vs Hash-V-grow | B_heap | 0 | 4 | 0 | 0 | 0 | 0 | 1.010 | 0 |
| MID | Hash-V-grow vs Hash-V-fixed | creation | 0 | 1 | 3 | 0 | 0 | 0 | 0.907 | 0 |
| MID | Hash-V-grow vs Hash-V-fixed | operation | 0 | 4 | 0 | 0 | 0 | 0 | 0.985 | 0 |
| MID | Hash-V-grow vs Hash-V-fixed | B_used | 4 | 0 | 0 | 0 | 0 | 0 | 0.444 | 0 |
| MID | Hash-V-grow vs Hash-V-fixed | B_heap | 4 | 0 | 0 | 0 | 0 | 0 | 0.475 | 0 |
| MID | Hash-M-grow vs Hash-M-fixed | creation | 0 | 0 | 4 | 0 | 0 | 0 | 0.869 | 0 |
| MID | Hash-M-grow vs Hash-M-fixed | operation | 0 | 4 | 0 | 0 | 0 | 0 | 0.982 | 0 |
| MID | Hash-M-grow vs Hash-M-fixed | B_used | 4 | 0 | 0 | 0 | 0 | 0 | 0.418 | 0 |
| MID | Hash-M-grow vs Hash-M-fixed | B_heap | 4 | 0 | 0 | 0 | 0 | 0 | 0.435 | 0 |
| MID | Hash-V-grow-map vs Hash-V-grow | creation | 0 | 4 | 0 | 0 | 0 | 0 | 1.024 | 0 |
| MID | Hash-V-grow-map vs Hash-V-grow | operation | 0 | 0 | 0 | 0 | 0 | 4 | 1.006 | 0 |
| MID | Hash-V-grow-map vs Hash-V-grow | B_used | 0 | 0 | 0 | 0 | 0 | 4 | 1.000 | 0 |
| MID | Hash-V-grow-map vs Hash-V-grow | B_heap | 0 | 0 | 0 | 0 | 0 | 4 | 1.002 | 0 |
| HIGH | Hash-V-fixed vs TC | creation | 0 | 0 | 4 | 0 | 0 | 0 | 0.712 | 0 |
| HIGH | Hash-V-fixed vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.890 | 0 |
| HIGH | Hash-V-fixed vs TC | B_used | 0 | 0 | 4 | 0 | 0 | 0 | 2.250 | 4 |
| HIGH | Hash-V-fixed vs TC | B_heap | 0 | 0 | 4 | 0 | 0 | 0 | 2.282 | 4 |
| HIGH | Hash-M-fixed vs TC | creation | 0 | 0 | 4 | 0 | 0 | 0 | 0.740 | 0 |
| HIGH | Hash-M-fixed vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.887 | 0 |
| HIGH | Hash-M-fixed vs TC | B_used | 0 | 0 | 4 | 0 | 0 | 0 | 2.417 | 4 |
| HIGH | Hash-M-fixed vs TC | B_heap | 0 | 0 | 4 | 0 | 0 | 0 | 2.450 | 4 |
| HIGH | Hash-V-grow vs TC | creation | 0 | 0 | 4 | 0 | 0 | 0 | 0.659 | 0 |
| HIGH | Hash-V-grow vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.877 | 0 |
| HIGH | Hash-V-grow vs TC | B_used | 0 | 3 | 1 | 0 | 0 | 0 | 0.943 | 0 |
| HIGH | Hash-V-grow vs TC | B_heap | 0 | 2 | 2 | 0 | 0 | 0 | 1.053 | 0 |
| HIGH | Hash-M-grow vs TC | creation | 0 | 0 | 4 | 0 | 0 | 0 | 0.671 | 0 |
| HIGH | Hash-M-grow vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.871 | 0 |
| HIGH | Hash-M-grow vs TC | B_used | 0 | 3 | 1 | 0 | 0 | 0 | 0.947 | 0 |
| HIGH | Hash-M-grow vs TC | B_heap | 0 | 2 | 2 | 0 | 0 | 0 | 1.058 | 0 |
| HIGH | Hash-M-fixed vs Hash-V-fixed | creation | 0 | 4 | 0 | 0 | 0 | 0 | 1.043 | 0 |
| HIGH | Hash-M-fixed vs Hash-V-fixed | operation | 0 | 4 | 0 | 0 | 0 | 0 | 0.995 | 0 |
| HIGH | Hash-M-fixed vs Hash-V-fixed | B_used | 0 | 3 | 1 | 0 | 0 | 0 | 1.074 | 0 |
| HIGH | Hash-M-fixed vs Hash-V-fixed | B_heap | 0 | 3 | 1 | 0 | 0 | 0 | 1.074 | 0 |
| HIGH | Hash-M-grow vs Hash-V-grow | creation | 0 | 4 | 0 | 0 | 0 | 0 | 1.018 | 0 |
| HIGH | Hash-M-grow vs Hash-V-grow | operation | 0 | 4 | 0 | 0 | 0 | 0 | 0.994 | 0 |
| HIGH | Hash-M-grow vs Hash-V-grow | B_used | 0 | 4 | 0 | 0 | 0 | 0 | 1.004 | 0 |
| HIGH | Hash-M-grow vs Hash-V-grow | B_heap | 0 | 4 | 0 | 0 | 0 | 0 | 1.004 | 0 |
| HIGH | Hash-V-grow vs Hash-V-fixed | creation | 0 | 3 | 1 | 0 | 0 | 0 | 0.935 | 0 |
| HIGH | Hash-V-grow vs Hash-V-fixed | operation | 0 | 4 | 0 | 0 | 0 | 0 | 0.983 | 0 |
| HIGH | Hash-V-grow vs Hash-V-fixed | B_used | 4 | 0 | 0 | 0 | 0 | 0 | 0.419 | 0 |
| HIGH | Hash-V-grow vs Hash-V-fixed | B_heap | 4 | 0 | 0 | 0 | 0 | 0 | 0.457 | 0 |
| HIGH | Hash-M-grow vs Hash-M-fixed | creation | 0 | 1 | 3 | 0 | 0 | 0 | 0.913 | 0 |
| HIGH | Hash-M-grow vs Hash-M-fixed | operation | 0 | 4 | 0 | 0 | 0 | 0 | 0.982 | 0 |
| HIGH | Hash-M-grow vs Hash-M-fixed | B_used | 4 | 0 | 0 | 0 | 0 | 0 | 0.392 | 0 |
| HIGH | Hash-M-grow vs Hash-M-fixed | B_heap | 4 | 0 | 0 | 0 | 0 | 0 | 0.415 | 0 |
| HIGH | Hash-V-grow-map vs Hash-V-grow | creation | 0 | 4 | 0 | 0 | 0 | 0 | 1.016 | 0 |
| HIGH | Hash-V-grow-map vs Hash-V-grow | operation | 0 | 0 | 0 | 0 | 0 | 4 | 1.008 | 0 |
| HIGH | Hash-V-grow-map vs Hash-V-grow | B_used | 0 | 0 | 0 | 0 | 0 | 4 | 1.000 | 0 |
| HIGH | Hash-V-grow-map vs Hash-V-grow | B_heap | 0 | 0 | 0 | 0 | 0 | 4 | 1.002 | 0 |

### Size x sharing grid (ratios T(reference)/T(row); presented, not judged)

Hash-V-fixed against TC, creation:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.601 | 0.614 | 0.632 | 0.655 |
| 32 | 0.665 | 0.657 | 0.696 | 0.713 |
| 64 | 0.672 | 0.689 | 0.716 | 0.719 |
| 128 | 0.659 | 0.686 | 0.689 | 0.711 |

Hash-V-fixed against TC, operation:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.883 | 0.875 | 0.840 | 0.849 |
| 32 | 0.896 | 0.897 | 0.894 | 0.882 |
| 64 | 0.901 | 0.902 | 0.902 | 0.897 |
| 128 | 0.906 | 0.902 | 0.904 | 0.914 |

Hash-M-fixed against TC, creation:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.639 | 0.664 | 0.675 | 0.678 |
| 32 | 0.700 | 0.685 | 0.734 | 0.749 |
| 64 | 0.699 | 0.728 | 0.754 | 0.765 |
| 128 | 0.679 | 0.710 | 0.727 | 0.731 |

Hash-M-fixed against TC, operation:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.858 | 0.857 | 0.839 | 0.844 |
| 32 | 0.887 | 0.891 | 0.890 | 0.879 |
| 64 | 0.893 | 0.899 | 0.897 | 0.895 |
| 128 | 0.885 | 0.889 | 0.894 | 0.899 |

Hash-V-grow against TC, creation:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.264 | 0.454 | 0.572 | 0.622 |
| 32 | 0.394 | 0.455 | 0.634 | 0.656 |
| 64 | 0.396 | 0.566 | 0.614 | 0.662 |
| 128 | 0.424 | 0.598 | 0.654 | 0.710 |

Hash-V-grow against TC, operation:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.850 | 0.849 | 0.816 | 0.820 |
| 32 | 0.881 | 0.883 | 0.876 | 0.867 |
| 64 | 0.892 | 0.896 | 0.893 | 0.887 |
| 128 | 0.904 | 0.901 | 0.909 | 0.899 |

Hash-M-grow against TC, creation:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.266 | 0.462 | 0.588 | 0.636 |
| 32 | 0.392 | 0.448 | 0.637 | 0.666 |
| 64 | 0.396 | 0.561 | 0.626 | 0.677 |
| 128 | 0.429 | 0.598 | 0.655 | 0.719 |

Hash-M-grow against TC, operation:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.826 | 0.825 | 0.811 | 0.813 |
| 32 | 0.868 | 0.873 | 0.872 | 0.860 |
| 64 | 0.880 | 0.883 | 0.884 | 0.882 |
| 128 | 0.889 | 0.896 | 0.895 | 0.901 |

Hash-M-fixed against Hash-V-fixed, creation:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 1.063 | 1.081 | 1.067 | 1.034 |
| 32 | 1.053 | 1.042 | 1.055 | 1.051 |
| 64 | 1.040 | 1.057 | 1.053 | 1.065 |
| 128 | 1.031 | 1.035 | 1.056 | 1.027 |

Hash-M-fixed against Hash-V-fixed, operation:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.972 | 0.979 | 0.999 | 0.993 |
| 32 | 0.991 | 0.993 | 0.995 | 0.996 |
| 64 | 0.991 | 0.997 | 0.995 | 0.997 |
| 128 | 0.977 | 0.986 | 0.988 | 0.984 |

Hash-M-grow against Hash-V-grow, creation:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 1.008 | 1.016 | 1.028 | 1.022 |
| 32 | 0.995 | 0.985 | 1.005 | 1.014 |
| 64 | 1.000 | 0.991 | 1.019 | 1.023 |
| 128 | 1.011 | 1.001 | 1.002 | 1.012 |

Hash-M-grow against Hash-V-grow, operation:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.972 | 0.971 | 0.994 | 0.992 |
| 32 | 0.985 | 0.989 | 0.995 | 0.992 |
| 64 | 0.987 | 0.986 | 0.990 | 0.995 |
| 128 | 0.983 | 0.994 | 0.984 | 1.003 |

Hash-V-grow against Hash-V-fixed, creation:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.439 | 0.740 | 0.904 | 0.949 |
| 32 | 0.592 | 0.693 | 0.911 | 0.921 |
| 64 | 0.590 | 0.821 | 0.858 | 0.921 |
| 128 | 0.644 | 0.872 | 0.949 | 0.999 |

Hash-V-grow against Hash-V-fixed, operation:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.962 | 0.970 | 0.972 | 0.965 |
| 32 | 0.984 | 0.985 | 0.980 | 0.983 |
| 64 | 0.990 | 0.993 | 0.990 | 0.988 |
| 128 | 0.998 | 0.999 | 1.006 | 0.984 |

Hash-M-grow against Hash-M-fixed, creation:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.417 | 0.695 | 0.871 | 0.938 |
| 32 | 0.559 | 0.655 | 0.868 | 0.888 |
| 64 | 0.567 | 0.770 | 0.830 | 0.885 |
| 128 | 0.632 | 0.843 | 0.901 | 0.984 |

Hash-M-grow against Hash-M-fixed, operation:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.962 | 0.963 | 0.967 | 0.964 |
| 32 | 0.978 | 0.980 | 0.979 | 0.978 |
| 64 | 0.986 | 0.982 | 0.985 | 0.986 |
| 128 | 1.004 | 1.008 | 1.002 | 1.003 |

Hash-V-grow-map against Hash-V-grow, creation:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 1.279 | 1.114 | 1.033 | 1.014 |
| 32 | 1.157 | 1.156 | 1.015 | 1.005 |
| 64 | 1.186 | 1.060 | 1.063 | 1.025 |
| 128 | 1.156 | 1.041 | 1.009 | 1.017 |

Hash-V-grow-map against Hash-V-grow, operation:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 1.004 | 1.000 | 1.009 | 1.011 |
| 32 | 1.005 | 1.004 | 1.011 | 1.005 |
| 64 | 1.000 | 1.001 | 1.002 | 1.007 |
| 128 | 0.998 | 1.002 | 0.997 | 1.008 |

## The growth peak (grow rows; reported, no verdict)

The most bytes per cell at any growth: accounted (every matrix's cells and table, the old table included, plus the grown table (n), the slot map (r) in Hash-V-grow-map, and tau) and heap (`mallinfo2`, read inside the growth just before the old table is freed), over 3n^2 cells; and Hash-V-grow-map over Hash-V-grow.

| job | Hash-V-grow accounted, heap (growths) | Hash-M-grow accounted, heap (growths) | Hash-V-grow-map accounted, heap (growths) | map / lookup: accounted, heap |
| --- | --- | --- | --- | --- |
| 1 (n 16, rho 1) | 41.33, 44.73 (18) | 44.00, 47.48 (18) | 42.67, 44.90 (18) | 1.032, 1.004 |
| 2 (n 16, rho 4) | 25.33, 28.73 (14) | 26.00, 29.48 (14) | 26.67, 28.90 (14) | 1.053, 1.006 |
| 3 (n 16, rho 16) | 21.33, 24.73 (10) | 21.50, 24.79 (10) | 22.67, 24.90 (10) | 1.063, 1.007 |
| 4 (n 16, rho 64) | 20.50, 23.71 (7) | 20.58, 23.88 (7) | 21.83, 23.88 (7) | 1.065, 1.007 |
| 5 (n 32, rho 1) | 30.67, 31.52 (22) | 32.00, 32.87 (22) | 32.00, 32.90 (22) | 1.043, 1.044 |
| 6 (n 32, rho 4) | 25.33, 26.18 (20) | 26.00, 26.87 (20) | 26.67, 27.56 (20) | 1.053, 1.053 |
| 7 (n 32, rho 16) | 21.33, 22.18 (16) | 21.50, 22.37 (16) | 22.67, 23.56 (16) | 1.063, 1.062 |
| 8 (n 32, rho 64) | 20.33, 21.18 (12) | 20.38, 21.20 (12) | 21.67, 22.56 (12) | 1.066, 1.065 |
| 9 (n 64, rho 1) | 30.67, 30.88 (28) | 32.00, 32.22 (28) | 32.00, 32.22 (28) | 1.043, 1.044 |
| 10 (n 64, rho 4) | 22.67, 22.88 (24) | 23.00, 23.22 (24) | 24.00, 24.22 (24) | 1.059, 1.059 |
| 11 (n 64, rho 16) | 21.33, 21.55 (22) | 21.50, 21.72 (22) | 22.67, 22.89 (22) | 1.063, 1.062 |
| 12 (n 64, rho 64) | 20.33, 20.55 (18) | 20.38, 20.59 (18) | 21.67, 21.89 (18) | 1.066, 1.065 |
| 13 (n 128, rho 1) | 34.67, 34.72 (34) | 37.33, 37.39 (34) | 37.33, 37.39 (34) | 1.077, 1.077 |
| 14 (n 128, rho 4) | 26.67, 26.72 (30) | 27.33, 27.39 (30) | 29.33, 29.39 (30) | 1.100, 1.100 |
| 15 (n 128, rho 16) | 24.67, 24.72 (26) | 24.83, 24.89 (26) | 27.33, 27.39 (26) | 1.108, 1.108 |
| 16 (n 128, rho 64) | 24.33, 24.39 (24) | 24.42, 24.47 (24) | 27.00, 27.06 (24) | 1.110, 1.109 |

## tau's build time (M rows; reported, no verdict)

The median over rounds of tau's build time per run, and its share of the operation part (the median of the per-round shares). tau is built, used and freed inside the operation part (spec section 8).

| job | Hash-M-fixed tau ns (share) | Hash-M-grow tau ns (share) |
| --- | --- | --- |
| 1 (n 16, rho 1) | 5402 (0.0108) | 5970 (0.0115) |
| 2 (n 16, rho 4) | 2829 (0.0057) | 1852 (0.0036) |
| 3 (n 16, rho 16) | 1639 (0.0037) | 495 (0.0011) |
| 4 (n 16, rho 64) | 1294 (0.0029) | 200 (0.0004) |
| 5 (n 32, rho 1) | 20738 (0.0050) | 20237 (0.0048) |
| 6 (n 32, rho 4) | 9778 (0.0024) | 6329 (0.0015) |
| 7 (n 32, rho 16) | 6079 (0.0015) | 1995 (0.0005) |
| 8 (n 32, rho 64) | 4905 (0.0012) | 621 (0.0002) |
| 9 (n 64, rho 1) | 79825 (0.0023) | 77046 (0.0022) |
| 10 (n 64, rho 4) | 37163 (0.0011) | 20576 (0.0006) |
| 11 (n 64, rho 16) | 22777 (0.0007) | 6399 (0.0002) |
| 12 (n 64, rho 64) | 19135 (0.0006) | 1973 (0.0001) |
| 13 (n 128, rho 1) | 262627 (0.0009) | 204075 (0.0007) |
| 14 (n 128, rho 4) | 162566 (0.0006) | 71578 (0.0003) |
| 15 (n 128, rho 16) | 155835 (0.0006) | 19854 (0.0001) |
| 16 (n 128, rho 64) | 134219 (0.0005) | 6889 (0.0000) |

## Stores, per job (rule-m1 item 13)

From the counting runs. Fixed: Hash-V-fixed (Hash-M-fixed has the same stores). Grow: Hash-V-grow (Hash-M-grow the same; Hash-V-grow-map the same but no rewrite probe). W counts 0 and 1. Probes per store over every whole-number store (0 and 1 found with no probe).

| job | sizing | store | when | W | T | fill | w | w_h | stores (found / new) | probes per store | longest | widenings (slots copied) | growths | reinserted (probes) | fields rewritten | rewrite probes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | fixed | A | after creation | 284 | 1024 | 27.7% | 64 | 16 | 230 / 282 | 1.02 | 5 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 1 | fixed | A | end | 284 | 1024 | 27.7% | 64 | 16 | 230 / 282 | 1.02 | 5 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 1 | fixed | B | after creation | 284 | 1024 | 27.7% | 64 | 16 | 230 / 282 | 1.00 | 3 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 1 | fixed | B | end | 284 | 1024 | 27.7% | 64 | 16 | 230 / 282 | 1.00 | 3 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 1 | fixed | C | after creation | 2 | 1024 | 0.2% | 8 | 16 | 0 / 0 | n/a | 0 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 1 | fixed | C | end | 313 | 1024 | 30.6% | 64 | 16 | 201 / 311 | 1.15 | 6 | 1 (1024) | 0 | 0 (0) | 0 | 0 |
| 1 | grow | A | after creation | 284 | 1024 | 27.7% | 64 | 16 | 230 / 282 | 1.29 | 7 | 0 (0) | 6 | 492 (562) | 810 | 840 |
| 1 | grow | A | end | 284 | 1024 | 27.7% | 64 | 16 | 230 / 282 | 1.29 | 7 | 0 (0) | 6 | 492 (562) | 810 | 840 |
| 1 | grow | B | after creation | 284 | 1024 | 27.7% | 64 | 16 | 230 / 282 | 1.36 | 10 | 0 (0) | 6 | 492 (549) | 804 | 828 |
| 1 | grow | B | end | 284 | 1024 | 27.7% | 64 | 16 | 230 / 282 | 1.36 | 10 | 0 (0) | 6 | 492 (549) | 804 | 828 |
| 1 | grow | C | after creation | 2 | 16 | 12.5% | 8 | 16 | 0 / 0 | n/a | 0 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 1 | grow | C | end | 313 | 1024 | 30.6% | 64 | 16 | 201 / 311 | 1.48 | 9 | 1 (16) | 6 | 492 (577) | 733 | 827 |
| 2 | fixed | A | after creation | 87 | 1024 | 8.5% | 64 | 16 | 427 / 85 | 0.98 | 2 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 2 | fixed | A | end | 87 | 1024 | 8.5% | 64 | 16 | 427 / 85 | 0.98 | 2 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 2 | fixed | B | after creation | 93 | 1024 | 9.1% | 64 | 16 | 421 / 91 | 0.96 | 2 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 2 | fixed | B | end | 93 | 1024 | 9.1% | 64 | 16 | 421 / 91 | 0.96 | 2 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 2 | fixed | C | after creation | 2 | 1024 | 0.2% | 8 | 16 | 0 / 0 | n/a | 0 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 2 | fixed | C | end | 314 | 1024 | 30.7% | 64 | 16 | 200 / 312 | 1.15 | 7 | 1 (1024) | 0 | 0 (0) | 0 | 0 |
| 2 | grow | A | after creation | 87 | 256 | 34.0% | 64 | 16 | 427 / 85 | 1.31 | 8 | 0 (0) | 4 | 112 (139) | 192 | 224 |
| 2 | grow | A | end | 87 | 256 | 34.0% | 64 | 16 | 427 / 85 | 1.31 | 8 | 0 (0) | 4 | 112 (139) | 192 | 224 |
| 2 | grow | B | after creation | 93 | 256 | 36.3% | 64 | 16 | 421 / 91 | 1.22 | 8 | 0 (0) | 4 | 112 (130) | 221 | 236 |
| 2 | grow | B | end | 93 | 256 | 36.3% | 64 | 16 | 421 / 91 | 1.22 | 8 | 0 (0) | 4 | 112 (130) | 221 | 236 |
| 2 | grow | C | after creation | 2 | 16 | 12.5% | 8 | 16 | 0 / 0 | n/a | 0 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 2 | grow | C | end | 314 | 1024 | 30.7% | 64 | 16 | 200 / 312 | 1.47 | 11 | 1 (16) | 6 | 492 (563) | 740 | 829 |
| 3 | fixed | A | after creation | 25 | 1024 | 2.4% | 64 | 16 | 489 / 23 | 0.94 | 1 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 3 | fixed | A | end | 25 | 1024 | 2.4% | 64 | 16 | 489 / 23 | 0.94 | 1 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 3 | fixed | B | after creation | 27 | 1024 | 2.6% | 64 | 16 | 487 / 25 | 0.95 | 1 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 3 | fixed | B | end | 27 | 1024 | 2.6% | 64 | 16 | 487 / 25 | 0.95 | 1 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 3 | fixed | C | after creation | 2 | 1024 | 0.2% | 8 | 16 | 0 / 0 | n/a | 0 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 3 | fixed | C | end | 317 | 1024 | 31.0% | 64 | 16 | 197 / 315 | 1.13 | 6 | 1 (1024) | 0 | 0 (0) | 0 | 0 |
| 3 | grow | A | after creation | 25 | 64 | 39.1% | 64 | 16 | 489 / 23 | 1.14 | 4 | 0 (0) | 2 | 20 (22) | 40 | 45 |
| 3 | grow | A | end | 25 | 64 | 39.1% | 64 | 16 | 489 / 23 | 1.14 | 4 | 0 (0) | 2 | 20 (22) | 40 | 45 |
| 3 | grow | B | after creation | 27 | 64 | 42.2% | 64 | 16 | 487 / 25 | 1.22 | 4 | 0 (0) | 2 | 20 (25) | 37 | 42 |
| 3 | grow | B | end | 27 | 64 | 42.2% | 64 | 16 | 487 / 25 | 1.22 | 4 | 0 (0) | 2 | 20 (25) | 37 | 42 |
| 3 | grow | C | after creation | 2 | 16 | 12.5% | 8 | 16 | 0 / 0 | n/a | 0 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 3 | grow | C | end | 317 | 1024 | 31.0% | 64 | 16 | 197 / 315 | 1.39 | 8 | 1 (16) | 6 | 492 (553) | 720 | 782 |
| 4 | fixed | A | after creation | 8 | 1024 | 0.8% | 64 | 16 | 506 / 6 | 0.74 | 1 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 4 | fixed | A | end | 8 | 1024 | 0.8% | 64 | 16 | 506 / 6 | 0.74 | 1 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 4 | fixed | B | after creation | 10 | 1024 | 1.0% | 64 | 16 | 504 / 8 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 4 | fixed | B | end | 10 | 1024 | 1.0% | 64 | 16 | 504 / 8 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 4 | fixed | C | after creation | 2 | 1024 | 0.2% | 8 | 16 | 0 / 0 | n/a | 0 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 4 | fixed | C | end | 296 | 1024 | 28.9% | 64 | 16 | 218 / 294 | 1.12 | 4 | 1 (1024) | 0 | 0 (0) | 0 | 0 |
| 4 | grow | A | after creation | 8 | 16 | 50.0% | 64 | 16 | 506 / 6 | 0.93 | 2 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 4 | grow | A | end | 8 | 16 | 50.0% | 64 | 16 | 506 / 6 | 0.93 | 2 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 4 | grow | B | after creation | 10 | 32 | 31.2% | 64 | 16 | 504 / 8 | 1.00 | 2 | 0 (0) | 1 | 6 (6) | 26 | 26 |
| 4 | grow | B | end | 10 | 32 | 31.2% | 64 | 16 | 504 / 8 | 1.00 | 2 | 0 (0) | 1 | 6 (6) | 26 | 26 |
| 4 | grow | C | after creation | 2 | 16 | 12.5% | 8 | 16 | 0 / 0 | n/a | 0 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 4 | grow | C | end | 296 | 1024 | 28.9% | 64 | 16 | 218 / 294 | 1.41 | 9 | 1 (16) | 6 | 492 (578) | 786 | 872 |
| 5 | fixed | A | after creation | 1007 | 4096 | 24.6% | 64 | 16 | 1043 / 1005 | 1.01 | 8 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 5 | fixed | A | end | 1007 | 4096 | 24.6% | 64 | 16 | 1043 / 1005 | 1.01 | 8 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 5 | fixed | B | after creation | 1017 | 4096 | 24.8% | 64 | 16 | 1033 / 1015 | 1.00 | 6 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 5 | fixed | B | end | 1017 | 4096 | 24.8% | 64 | 16 | 1033 / 1015 | 1.00 | 6 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 5 | fixed | C | after creation | 2 | 4096 | 0.0% | 8 | 16 | 0 / 0 | n/a | 0 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 5 | fixed | C | end | 1096 | 4096 | 26.8% | 64 | 16 | 954 / 1094 | 1.11 | 6 | 1 (4096) | 0 | 0 (0) | 0 | 0 |
| 5 | grow | A | after creation | 1007 | 2048 | 49.2% | 64 | 16 | 1043 / 1005 | 1.30 | 11 | 0 (0) | 7 | 1002 (1144) | 1792 | 1864 |
| 5 | grow | A | end | 1007 | 2048 | 49.2% | 64 | 16 | 1043 / 1005 | 1.30 | 11 | 0 (0) | 7 | 1002 (1144) | 1792 | 1864 |
| 5 | grow | B | after creation | 1017 | 2048 | 49.7% | 64 | 16 | 1033 / 1015 | 1.37 | 24 | 0 (0) | 7 | 1002 (1131) | 1764 | 1831 |
| 5 | grow | B | end | 1017 | 2048 | 49.7% | 64 | 16 | 1033 / 1015 | 1.37 | 24 | 0 (0) | 7 | 1002 (1131) | 1764 | 1831 |
| 5 | grow | C | after creation | 2 | 16 | 12.5% | 8 | 16 | 0 / 0 | n/a | 0 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 5 | grow | C | end | 1096 | 4096 | 26.8% | 64 | 16 | 954 / 1094 | 1.46 | 12 | 1 (16) | 8 | 2024 (2416) | 3541 | 3942 |
| 6 | fixed | A | after creation | 284 | 4096 | 6.9% | 64 | 16 | 1766 / 282 | 0.98 | 4 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 6 | fixed | A | end | 284 | 4096 | 6.9% | 64 | 16 | 1766 / 282 | 0.98 | 4 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 6 | fixed | B | after creation | 281 | 4096 | 6.9% | 64 | 16 | 1769 / 279 | 0.96 | 2 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 6 | fixed | B | end | 281 | 4096 | 6.9% | 64 | 16 | 1769 / 279 | 0.96 | 2 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 6 | fixed | C | after creation | 2 | 4096 | 0.0% | 8 | 16 | 0 / 0 | n/a | 0 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 6 | fixed | C | end | 1104 | 4096 | 27.0% | 64 | 16 | 946 / 1102 | 1.09 | 5 | 1 (4096) | 0 | 0 (0) | 0 | 0 |
| 6 | grow | A | after creation | 284 | 1024 | 27.7% | 64 | 16 | 1766 / 282 | 1.20 | 9 | 0 (0) | 6 | 492 (592) | 1450 | 1573 |
| 6 | grow | A | end | 284 | 1024 | 27.7% | 64 | 16 | 1766 / 282 | 1.20 | 9 | 0 (0) | 6 | 492 (592) | 1450 | 1573 |
| 6 | grow | B | after creation | 281 | 1024 | 27.4% | 64 | 16 | 1769 / 279 | 1.26 | 36 | 0 (0) | 6 | 492 (567) | 1524 | 1593 |
| 6 | grow | B | end | 281 | 1024 | 27.4% | 64 | 16 | 1769 / 279 | 1.26 | 36 | 0 (0) | 6 | 492 (567) | 1524 | 1593 |
| 6 | grow | C | after creation | 2 | 16 | 12.5% | 8 | 16 | 0 / 0 | n/a | 0 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 6 | grow | C | end | 1104 | 4096 | 27.0% | 64 | 16 | 946 / 1102 | 1.36 | 12 | 1 (16) | 8 | 2024 (2323) | 3522 | 3828 |
| 7 | fixed | A | after creation | 90 | 4096 | 2.2% | 64 | 16 | 1960 / 88 | 0.99 | 2 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 7 | fixed | A | end | 90 | 4096 | 2.2% | 64 | 16 | 1960 / 88 | 0.99 | 2 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 7 | fixed | B | after creation | 87 | 4096 | 2.1% | 64 | 16 | 1963 / 85 | 0.94 | 1 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 7 | fixed | B | end | 87 | 4096 | 2.1% | 64 | 16 | 1963 / 85 | 0.94 | 1 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 7 | fixed | C | after creation | 2 | 4096 | 0.0% | 8 | 16 | 0 / 0 | n/a | 0 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 7 | fixed | C | end | 1104 | 4096 | 27.0% | 64 | 16 | 946 / 1102 | 1.10 | 6 | 1 (4096) | 0 | 0 (0) | 0 | 0 |
| 7 | grow | A | after creation | 90 | 256 | 35.2% | 64 | 16 | 1960 / 88 | 1.09 | 5 | 0 (0) | 4 | 112 (120) | 213 | 219 |
| 7 | grow | A | end | 90 | 256 | 35.2% | 64 | 16 | 1960 / 88 | 1.09 | 5 | 0 (0) | 4 | 112 (120) | 213 | 219 |
| 7 | grow | B | after creation | 87 | 256 | 34.0% | 64 | 16 | 1963 / 85 | 1.17 | 11 | 0 (0) | 4 | 112 (134) | 217 | 235 |
| 7 | grow | B | end | 87 | 256 | 34.0% | 64 | 16 | 1963 / 85 | 1.17 | 11 | 0 (0) | 4 | 112 (134) | 217 | 235 |
| 7 | grow | C | after creation | 2 | 16 | 12.5% | 8 | 16 | 0 / 0 | n/a | 0 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 7 | grow | C | end | 1104 | 4096 | 27.0% | 64 | 16 | 946 / 1102 | 1.52 | 20 | 1 (16) | 8 | 2024 (2365) | 3540 | 3905 |
| 8 | fixed | A | after creation | 30 | 4096 | 0.7% | 64 | 16 | 2020 / 28 | 0.94 | 1 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 8 | fixed | A | end | 30 | 4096 | 0.7% | 64 | 16 | 2020 / 28 | 0.94 | 1 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 8 | fixed | B | after creation | 28 | 4096 | 0.7% | 64 | 16 | 2022 / 26 | 0.94 | 1 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 8 | fixed | B | end | 28 | 4096 | 0.7% | 64 | 16 | 2022 / 26 | 0.94 | 1 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 8 | fixed | C | after creation | 2 | 4096 | 0.0% | 8 | 16 | 0 / 0 | n/a | 0 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 8 | fixed | C | end | 1088 | 4096 | 26.6% | 64 | 16 | 962 / 1086 | 1.08 | 5 | 1 (4096) | 0 | 0 (0) | 0 | 0 |
| 8 | grow | A | after creation | 30 | 64 | 46.9% | 64 | 16 | 2020 / 28 | 1.25 | 5 | 0 (0) | 2 | 20 (22) | 36 | 37 |
| 8 | grow | A | end | 30 | 64 | 46.9% | 64 | 16 | 2020 / 28 | 1.25 | 5 | 0 (0) | 2 | 20 (22) | 36 | 37 |
| 8 | grow | B | after creation | 28 | 64 | 43.8% | 64 | 16 | 2022 / 26 | 1.20 | 4 | 0 (0) | 2 | 20 (22) | 36 | 34 |
| 8 | grow | B | end | 28 | 64 | 43.8% | 64 | 16 | 2022 / 26 | 1.20 | 4 | 0 (0) | 2 | 20 (22) | 36 | 34 |
| 8 | grow | C | after creation | 2 | 16 | 12.5% | 8 | 16 | 0 / 0 | n/a | 0 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 8 | grow | C | end | 1088 | 4096 | 26.6% | 64 | 16 | 962 / 1086 | 1.38 | 11 | 1 (16) | 8 | 2024 (2313) | 3642 | 3952 |
| 9 | fixed | A | after creation | 3534 | 16384 | 21.6% | 64 | 16 | 4660 / 3532 | 0.97 | 4 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 9 | fixed | A | end | 3534 | 16384 | 21.6% | 64 | 16 | 4660 / 3532 | 0.97 | 4 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 9 | fixed | B | after creation | 3590 | 16384 | 21.9% | 64 | 16 | 4604 / 3588 | 0.97 | 4 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 9 | fixed | B | end | 3590 | 16384 | 21.9% | 64 | 16 | 4604 / 3588 | 0.97 | 4 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 9 | fixed | C | after creation | 2 | 16384 | 0.0% | 8 | 16 | 0 / 0 | n/a | 0 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 9 | fixed | C | end | 4190 | 16384 | 25.6% | 64 | 16 | 4004 / 4188 | 1.10 | 6 | 1 (16384) | 0 | 0 (0) | 0 | 0 |
| 9 | grow | A | after creation | 3534 | 8192 | 43.1% | 64 | 16 | 4660 / 3532 | 1.21 | 21 | 0 (0) | 9 | 4070 (4535) | 8322 | 8277 |
| 9 | grow | A | end | 3534 | 8192 | 43.1% | 64 | 16 | 4660 / 3532 | 1.21 | 21 | 0 (0) | 9 | 4070 (4535) | 8322 | 8277 |
| 9 | grow | B | after creation | 3590 | 8192 | 43.8% | 64 | 16 | 4604 / 3588 | 1.19 | 12 | 0 (0) | 9 | 4070 (4541) | 8159 | 8250 |
| 9 | grow | B | end | 3590 | 8192 | 43.8% | 64 | 16 | 4604 / 3588 | 1.19 | 12 | 0 (0) | 9 | 4070 (4541) | 8159 | 8250 |
| 9 | grow | C | after creation | 2 | 16 | 12.5% | 8 | 16 | 0 / 0 | n/a | 0 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 9 | grow | C | end | 4190 | 16384 | 25.6% | 64 | 16 | 4004 / 4188 | 1.44 | 20 | 1 (16) | 10 | 8164 (9640) | 15522 | 17014 |
| 10 | fixed | A | after creation | 1014 | 16384 | 6.2% | 64 | 16 | 7180 / 1012 | 0.95 | 2 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 10 | fixed | A | end | 1014 | 16384 | 6.2% | 64 | 16 | 7180 / 1012 | 0.95 | 2 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 10 | fixed | B | after creation | 1007 | 16384 | 6.1% | 64 | 16 | 7187 / 1005 | 0.95 | 2 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 10 | fixed | B | end | 1007 | 16384 | 6.1% | 64 | 16 | 7187 / 1005 | 0.95 | 2 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 10 | fixed | C | after creation | 2 | 16384 | 0.0% | 8 | 16 | 0 / 0 | n/a | 0 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 10 | fixed | C | end | 4191 | 16384 | 25.6% | 64 | 16 | 4003 / 4189 | 1.09 | 7 | 1 (16384) | 0 | 0 (0) | 0 | 0 |
| 10 | grow | A | after creation | 1014 | 2048 | 49.5% | 64 | 16 | 7180 / 1012 | 1.15 | 11 | 0 (0) | 7 | 1002 (1137) | 2168 | 2195 |
| 10 | grow | A | end | 1014 | 2048 | 49.5% | 64 | 16 | 7180 / 1012 | 1.15 | 11 | 0 (0) | 7 | 1002 (1137) | 2168 | 2195 |
| 10 | grow | B | after creation | 1007 | 2048 | 49.2% | 64 | 16 | 7187 / 1005 | 1.16 | 14 | 0 (0) | 7 | 1002 (1139) | 2190 | 2220 |
| 10 | grow | B | end | 1007 | 2048 | 49.2% | 64 | 16 | 7187 / 1005 | 1.16 | 14 | 0 (0) | 7 | 1002 (1139) | 2190 | 2220 |
| 10 | grow | C | after creation | 2 | 16 | 12.5% | 8 | 16 | 0 / 0 | n/a | 0 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 10 | grow | C | end | 4191 | 16384 | 25.6% | 64 | 16 | 4003 / 4189 | 1.50 | 20 | 1 (16) | 10 | 8164 (9508) | 15529 | 17548 |
| 11 | fixed | A | after creation | 281 | 16384 | 1.7% | 64 | 16 | 7913 / 279 | 0.94 | 2 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 11 | fixed | A | end | 281 | 16384 | 1.7% | 64 | 16 | 7913 / 279 | 0.94 | 2 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 11 | fixed | B | after creation | 284 | 16384 | 1.7% | 64 | 16 | 7910 / 282 | 0.96 | 2 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 11 | fixed | B | end | 284 | 16384 | 1.7% | 64 | 16 | 7910 / 282 | 0.96 | 2 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 11 | fixed | C | after creation | 2 | 16384 | 0.0% | 8 | 16 | 0 / 0 | n/a | 0 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 11 | fixed | C | end | 4191 | 16384 | 25.6% | 64 | 16 | 4003 / 4189 | 1.09 | 9 | 1 (16384) | 0 | 0 (0) | 0 | 0 |
| 11 | grow | A | after creation | 281 | 1024 | 27.4% | 64 | 16 | 7913 / 279 | 1.10 | 13 | 0 (0) | 6 | 492 (603) | 1462 | 1542 |
| 11 | grow | A | end | 281 | 1024 | 27.4% | 64 | 16 | 7913 / 279 | 1.10 | 13 | 0 (0) | 6 | 492 (603) | 1462 | 1542 |
| 11 | grow | B | after creation | 284 | 1024 | 27.7% | 64 | 16 | 7910 / 282 | 1.10 | 12 | 0 (0) | 6 | 492 (574) | 1823 | 1906 |
| 11 | grow | B | end | 284 | 1024 | 27.7% | 64 | 16 | 7910 / 282 | 1.10 | 12 | 0 (0) | 6 | 492 (574) | 1823 | 1906 |
| 11 | grow | C | after creation | 2 | 16 | 12.5% | 8 | 16 | 0 / 0 | n/a | 0 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 11 | grow | C | end | 4191 | 16384 | 25.6% | 64 | 16 | 4003 / 4189 | 1.44 | 19 | 1 (16) | 10 | 8164 (9535) | 15553 | 16983 |
| 12 | fixed | A | after creation | 91 | 16384 | 0.6% | 64 | 16 | 8103 / 89 | 0.94 | 1 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 12 | fixed | A | end | 91 | 16384 | 0.6% | 64 | 16 | 8103 / 89 | 0.94 | 1 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 12 | fixed | B | after creation | 85 | 16384 | 0.5% | 64 | 16 | 8109 / 83 | 0.91 | 1 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 12 | fixed | B | end | 85 | 16384 | 0.5% | 64 | 16 | 8109 / 83 | 0.91 | 1 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 12 | fixed | C | after creation | 2 | 16384 | 0.0% | 8 | 16 | 0 / 0 | n/a | 0 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 12 | fixed | C | end | 4200 | 16384 | 25.6% | 64 | 16 | 3994 / 4198 | 1.09 | 6 | 1 (16384) | 0 | 0 (0) | 0 | 0 |
| 12 | grow | A | after creation | 91 | 256 | 35.5% | 64 | 16 | 8103 / 89 | 1.19 | 5 | 0 (0) | 4 | 112 (135) | 232 | 255 |
| 12 | grow | A | end | 91 | 256 | 35.5% | 64 | 16 | 8103 / 89 | 1.19 | 5 | 0 (0) | 4 | 112 (135) | 232 | 255 |
| 12 | grow | B | after creation | 85 | 256 | 33.2% | 64 | 16 | 8109 / 83 | 1.14 | 15 | 0 (0) | 4 | 112 (133) | 224 | 247 |
| 12 | grow | B | end | 85 | 256 | 33.2% | 64 | 16 | 8109 / 83 | 1.14 | 15 | 0 (0) | 4 | 112 (133) | 224 | 247 |
| 12 | grow | C | after creation | 2 | 16 | 12.5% | 8 | 16 | 0 / 0 | n/a | 0 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 12 | grow | C | end | 4200 | 16384 | 25.6% | 64 | 16 | 3994 / 4198 | 1.43 | 25 | 1 (16) | 10 | 8164 (9533) | 15498 | 16910 |
| 13 | fixed | A | after creation | 10109 | 65536 | 15.4% | 64 | 32 | 22661 / 10107 | 0.94 | 1 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 13 | fixed | A | end | 10109 | 65536 | 15.4% | 64 | 32 | 22661 / 10107 | 0.94 | 1 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 13 | fixed | B | after creation | 10101 | 65536 | 15.4% | 64 | 32 | 22669 / 10099 | 0.95 | 1 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 13 | fixed | B | end | 10101 | 65536 | 15.4% | 64 | 32 | 22669 / 10099 | 0.95 | 1 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 13 | fixed | C | after creation | 2 | 65536 | 0.0% | 8 | 32 | 0 / 0 | n/a | 0 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 13 | fixed | C | end | 16487 | 65536 | 25.2% | 64 | 32 | 16283 / 16485 | 1.08 | 11 | 1 (65536) | 0 | 0 (0) | 0 | 0 |
| 13 | grow | A | after creation | 10109 | 32768 | 30.9% | 64 | 32 | 22661 / 10107 | 1.08 | 19 | 0 (0) | 11 | 16354 (17334) | 41907 | 40566 |
| 13 | grow | A | end | 10109 | 32768 | 30.9% | 64 | 32 | 22661 / 10107 | 1.08 | 19 | 0 (0) | 11 | 16354 (17334) | 41907 | 40566 |
| 13 | grow | B | after creation | 10101 | 32768 | 30.8% | 64 | 32 | 22669 / 10099 | 1.08 | 24 | 0 (0) | 11 | 16354 (17296) | 41697 | 40450 |
| 13 | grow | B | end | 10101 | 32768 | 30.8% | 64 | 32 | 22669 / 10099 | 1.08 | 24 | 0 (0) | 11 | 16354 (17296) | 41697 | 40450 |
| 13 | grow | C | after creation | 2 | 16 | 12.5% | 8 | 32 | 0 / 0 | n/a | 0 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 13 | grow | C | end | 16487 | 65536 | 25.2% | 64 | 32 | 16283 / 16485 | 1.43 | 21 | 1 (16) | 12 | 32736 (38169) | 64292 | 69768 |
| 14 | fixed | A | after creation | 3516 | 65536 | 5.4% | 64 | 32 | 29254 / 3514 | 0.94 | 1 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 14 | fixed | A | end | 3516 | 65536 | 5.4% | 64 | 32 | 29254 / 3514 | 0.94 | 1 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 14 | fixed | B | after creation | 3534 | 65536 | 5.4% | 64 | 32 | 29236 / 3532 | 0.94 | 1 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 14 | fixed | B | end | 3534 | 65536 | 5.4% | 64 | 32 | 29236 / 3532 | 0.94 | 1 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 14 | fixed | C | after creation | 2 | 65536 | 0.0% | 8 | 32 | 0 / 0 | n/a | 0 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 14 | fixed | C | end | 16496 | 65536 | 25.2% | 64 | 32 | 16274 / 16494 | 1.09 | 9 | 1 (65536) | 0 | 0 (0) | 0 | 0 |
| 14 | grow | A | after creation | 3516 | 8192 | 42.9% | 64 | 32 | 29254 / 3514 | 1.07 | 11 | 0 (0) | 9 | 4070 (4573) | 10394 | 10382 |
| 14 | grow | A | end | 3516 | 8192 | 42.9% | 64 | 32 | 29254 / 3514 | 1.07 | 11 | 0 (0) | 9 | 4070 (4573) | 10394 | 10382 |
| 14 | grow | B | after creation | 3534 | 8192 | 43.1% | 64 | 32 | 29236 / 3532 | 1.08 | 17 | 0 (0) | 9 | 4070 (4528) | 10470 | 10422 |
| 14 | grow | B | end | 3534 | 8192 | 43.1% | 64 | 32 | 29236 / 3532 | 1.08 | 17 | 0 (0) | 9 | 4070 (4528) | 10470 | 10422 |
| 14 | grow | C | after creation | 2 | 16 | 12.5% | 8 | 32 | 0 / 0 | n/a | 0 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 14 | grow | C | end | 16496 | 65536 | 25.2% | 64 | 32 | 16274 / 16494 | 1.41 | 24 | 1 (16) | 12 | 32736 (38328) | 64259 | 69881 |
| 15 | fixed | A | after creation | 1009 | 65536 | 1.5% | 64 | 32 | 31761 / 1007 | 0.95 | 1 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 15 | fixed | A | end | 1009 | 65536 | 1.5% | 64 | 32 | 31761 / 1007 | 0.95 | 1 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 15 | fixed | B | after creation | 1016 | 65536 | 1.6% | 64 | 32 | 31754 / 1014 | 0.95 | 1 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 15 | fixed | B | end | 1016 | 65536 | 1.6% | 64 | 32 | 31754 / 1014 | 0.95 | 1 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 15 | fixed | C | after creation | 2 | 65536 | 0.0% | 8 | 32 | 0 / 0 | n/a | 0 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 15 | fixed | C | end | 16497 | 65536 | 25.2% | 64 | 32 | 16273 / 16495 | 1.09 | 10 | 1 (65536) | 0 | 0 (0) | 0 | 0 |
| 15 | grow | A | after creation | 1009 | 2048 | 49.3% | 64 | 32 | 31761 / 1007 | 1.16 | 13 | 0 (0) | 7 | 1002 (1157) | 2207 | 2278 |
| 15 | grow | A | end | 1009 | 2048 | 49.3% | 64 | 32 | 31761 / 1007 | 1.16 | 13 | 0 (0) | 7 | 1002 (1157) | 2207 | 2278 |
| 15 | grow | B | after creation | 1016 | 2048 | 49.6% | 64 | 32 | 31754 / 1014 | 1.19 | 16 | 0 (0) | 7 | 1002 (1179) | 2161 | 2263 |
| 15 | grow | B | end | 1016 | 2048 | 49.6% | 64 | 32 | 31754 / 1014 | 1.19 | 16 | 0 (0) | 7 | 1002 (1179) | 2161 | 2263 |
| 15 | grow | C | after creation | 2 | 16 | 12.5% | 8 | 32 | 0 / 0 | n/a | 0 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 15 | grow | C | end | 16497 | 65536 | 25.2% | 64 | 32 | 16273 / 16495 | 1.43 | 30 | 1 (16) | 12 | 32736 (38317) | 64303 | 69917 |
| 16 | fixed | A | after creation | 285 | 65536 | 0.4% | 64 | 32 | 32485 / 283 | 0.95 | 1 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 16 | fixed | A | end | 285 | 65536 | 0.4% | 64 | 32 | 32485 / 283 | 0.95 | 1 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 16 | fixed | B | after creation | 285 | 65536 | 0.4% | 64 | 32 | 32485 / 283 | 0.93 | 1 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 16 | fixed | B | end | 285 | 65536 | 0.4% | 64 | 32 | 32485 / 283 | 0.93 | 1 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 16 | fixed | C | after creation | 2 | 65536 | 0.0% | 8 | 32 | 0 / 0 | n/a | 0 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 16 | fixed | C | end | 16496 | 65536 | 25.2% | 64 | 32 | 16274 / 16494 | 1.08 | 10 | 1 (65536) | 0 | 0 (0) | 0 | 0 |
| 16 | grow | A | after creation | 285 | 1024 | 27.8% | 64 | 32 | 32485 / 283 | 1.04 | 13 | 0 (0) | 6 | 492 (559) | 1559 | 1599 |
| 16 | grow | A | end | 285 | 1024 | 27.8% | 64 | 32 | 32485 / 283 | 1.04 | 13 | 0 (0) | 6 | 492 (559) | 1559 | 1599 |
| 16 | grow | B | after creation | 285 | 1024 | 27.8% | 64 | 32 | 32485 / 283 | 1.02 | 15 | 0 (0) | 6 | 492 (560) | 1574 | 1576 |
| 16 | grow | B | end | 285 | 1024 | 27.8% | 64 | 32 | 32485 / 283 | 1.02 | 15 | 0 (0) | 6 | 492 (560) | 1574 | 1576 |
| 16 | grow | C | after creation | 2 | 16 | 12.5% | 8 | 32 | 0 / 0 | n/a | 0 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 16 | grow | C | end | 16496 | 65536 | 25.2% | 64 | 32 | 16274 / 16494 | 1.41 | 19 | 1 (16) | 12 | 32736 (38181) | 64279 | 69733 |

### tau's finds (M rows; from the counting runs)

| job | row | found | none | probes | position bytes |
| --- | --- | --- | --- | --- | --- |
| 1 | Hash-M-fixed | 33 | 249 | 384 | 2048 |
| 1 | Hash-M-grow | 33 | 249 | 384 | 2048 |
| 2 | Hash-M-fixed | 22 | 69 | 97 | 2048 |
| 2 | Hash-M-grow | 22 | 69 | 147 | 512 |
| 3 | Hash-M-fixed | 4 | 21 | 25 | 2048 |
| 3 | Hash-M-grow | 4 | 21 | 41 | 128 |
| 4 | Hash-M-fixed | 0 | 8 | 8 | 2048 |
| 4 | Hash-M-grow | 0 | 8 | 17 | 64 |
| 5 | Hash-M-fixed | 101 | 914 | 1287 | 8192 |
| 5 | Hash-M-grow | 101 | 914 | 2049 | 4096 |
| 6 | Hash-M-fixed | 38 | 241 | 301 | 8192 |
| 6 | Hash-M-grow | 38 | 241 | 396 | 2048 |
| 7 | Hash-M-fixed | 19 | 66 | 86 | 8192 |
| 7 | Hash-M-grow | 19 | 66 | 126 | 512 |
| 8 | Hash-M-fixed | 6 | 20 | 26 | 8192 |
| 8 | Hash-M-grow | 6 | 20 | 46 | 128 |
| 9 | Hash-M-fixed | 906 | 2682 | 3964 | 32768 |
| 9 | Hash-M-grow | 906 | 2682 | 5436 | 16384 |
| 10 | Hash-M-fixed | 110 | 895 | 1032 | 32768 |
| 10 | Hash-M-grow | 110 | 895 | 2071 | 4096 |
| 11 | Hash-M-fixed | 30 | 252 | 287 | 32768 |
| 11 | Hash-M-grow | 30 | 252 | 394 | 2048 |
| 12 | Hash-M-fixed | 17 | 66 | 83 | 32768 |
| 12 | Hash-M-grow | 17 | 66 | 115 | 512 |
| 13 | Hash-M-fixed | 5323 | 4776 | 10099 | 262144 |
| 13 | Hash-M-grow | 5323 | 4776 | 10727 | 131072 |
| 14 | Hash-M-fixed | 921 | 2611 | 3532 | 262144 |
| 14 | Hash-M-grow | 921 | 2611 | 5326 | 32768 |
| 15 | Hash-M-fixed | 107 | 907 | 1014 | 262144 |
| 15 | Hash-M-grow | 107 | 907 | 2130 | 8192 |
| 16 | Hash-M-fixed | 36 | 247 | 283 | 262144 |
| 16 | Hash-M-grow | 36 | 247 | 409 | 4096 |

## Also reported (rule-m1 item 13)

### Realized sharing

s: distinct values per matrix of A and B, as drawn; W per store (0 and 1 included) at the end, from the model (the gates checked it).

| job | n | rho | s | distinct A | distinct B | distinct C | W_A | W_B | W_C |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | 16 | 1 | 256 | 256 | 256 | 256 | 284 | 284 | 313 |
| 2 | 16 | 4 | 64 | 64 | 64 | 256 | 87 | 93 | 314 |
| 3 | 16 | 16 | 16 | 16 | 16 | 256 | 25 | 27 | 317 |
| 4 | 16 | 64 | 4 | 4 | 4 | 256 | 8 | 10 | 296 |
| 5 | 32 | 1 | 1024 | 1024 | 1024 | 1024 | 1007 | 1017 | 1096 |
| 6 | 32 | 4 | 256 | 256 | 256 | 1024 | 284 | 281 | 1104 |
| 7 | 32 | 16 | 64 | 64 | 64 | 1024 | 90 | 87 | 1104 |
| 8 | 32 | 64 | 16 | 16 | 16 | 1024 | 30 | 28 | 1088 |
| 9 | 64 | 1 | 4096 | 4096 | 4096 | 4096 | 3534 | 3590 | 4190 |
| 10 | 64 | 4 | 1024 | 1024 | 1024 | 4096 | 1014 | 1007 | 4191 |
| 11 | 64 | 16 | 256 | 256 | 256 | 4096 | 281 | 284 | 4191 |
| 12 | 64 | 64 | 64 | 64 | 64 | 4096 | 91 | 85 | 4200 |
| 13 | 128 | 1 | 16384 | 16384 | 16384 | 16384 | 10109 | 10101 | 16487 |
| 14 | 128 | 4 | 4096 | 4096 | 4096 | 16384 | 3516 | 3534 | 16496 |
| 15 | 128 | 16 | 1024 | 1024 | 1024 | 16384 | 1009 | 1016 | 16497 |
| 16 | 128 | 64 | 256 | 256 | 256 | 16384 | 285 | 285 | 16496 |

### Cases per operation (every row takes the same; spec section 10 items 3-4)

- job 1: add (4096): zero_x 0.062, int_x 0.001, int_y 0.037, den 0.003, general 0.896; mul (4096): int_both 0.011, den 0.040, int_x 0.106, int_y 0.095, general 0.749
- job 2: add (4096): zero_x 0.062, int_both 0.000, int_x 0.005, int_y 0.033, den 0.003, general 0.896; mul (4096): int_both 0.007, den 0.038, cross_ad 0.001, int_x 0.056, int_y 0.103, general 0.796
- job 3: add (4096): zero_x 0.062, int_x 0.005, int_y 0.112, den 0.005, general 0.816; mul (4096): int_both 0.014, den 0.074, int_x 0.103, int_y 0.095, general 0.714
- job 4: add (4096): zero_x 0.062, int_x 0.001, den 0.026, general 0.911; mul (4096): int_x 0.516, general 0.484
- job 5: add (32768): zero_x 0.031, int_x 0.001, int_y 0.047, den 0.002, general 0.919; mul (32768): int_both 0.012, den 0.036, num 0.000, cross_ad 0.000, cross_cb 0.000, int_x 0.098, int_y 0.105, general 0.750
- job 6: add (32768): zero_x 0.031, int_x 0.003, int_y 0.043, den 0.003, general 0.920; mul (32768): int_both 0.013, den 0.035, num 0.000, cross_ad 0.000, int_x 0.088, int_y 0.112, general 0.751
- job 7: add (32768): zero_x 0.031, int_x 0.001, int_y 0.031, den 0.003, general 0.934; mul (32768): int_both 0.005, den 0.036, int_x 0.039, int_y 0.121, general 0.799
- job 8: add (32768): zero_x 0.031, int_both 0.000, int_x 0.002, int_y 0.036, den 0.004, general 0.926; mul (32768): int_both 0.013, den 0.040, int_x 0.104, int_y 0.108, general 0.734
- job 9: add (262144): zero_x 0.016, int_both 0.000, int_x 0.001, int_y 0.042, den 0.002, general 0.939; mul (262144): int_both 0.012, den 0.036, num 0.000, cross_ad 0.000, cross_cb 0.000, int_x 0.105, int_y 0.090, general 0.757
- job 10: add (262144): zero_x 0.016, int_both 0.000, int_x 0.001, int_y 0.049, den 0.002, general 0.932; mul (262144): int_both 0.014, den 0.034, num 0.000, cross_cb 0.000, int_x 0.101, int_y 0.111, general 0.739
- job 11: add (262144): zero_x 0.016, int_both 0.000, int_x 0.001, int_y 0.042, den 0.001, general 0.940; mul (262144): int_both 0.012, den 0.036, num 0.000, cross_cb 0.000, int_x 0.114, int_y 0.080, general 0.758
- job 12: add (262144): zero_x 0.016, int_both 0.000, int_x 0.001, int_y 0.058, den 0.002, general 0.924; mul (262144): int_both 0.022, den 0.030, int_x 0.097, int_y 0.162, general 0.689
- job 13: add (2097152): zero_x 0.008, zero_y 0.000, int_both 0.000, int_x 0.000, int_y 0.042, den 0.001, general 0.948; mul (2097152): zero 0.000, one_y 0.000, int_both 0.012, den 0.035, num 0.000, cross_ad 0.000, cross_cb 0.000, int_x 0.100, int_y 0.095, general 0.757
- job 14: add (2097152): zero_x 0.008, int_both 0.000, int_x 0.000, int_y 0.044, den 0.001, general 0.946; mul (2097152): one_x 0.000, int_both 0.013, den 0.035, num 0.000, cross_ad 0.000, cross_cb 0.000, int_x 0.098, int_y 0.105, general 0.748
- job 15: add (2097152): zero_x 0.008, int_both 0.000, int_x 0.000, int_y 0.037, den 0.001, general 0.953; mul (2097152): int_both 0.011, den 0.035, num 0.000, cross_ad 0.000, cross_cb 0.000, int_x 0.093, int_y 0.093, general 0.768
- job 16: add (2097152): zero_x 0.008, int_both 0.000, int_x 0.000, int_y 0.048, den 0.001, general 0.943; mul (2097152): int_both 0.015, den 0.031, num 0.000, int_x 0.092, int_y 0.126, general 0.736

### Cross tests per operation (counting runs)

Four cross tests per operation on two cells (spec section 6); V reads the values (cross reads), M compares handles through tau and reads only what a case needs (K-Q3 reads).

| job | operations on cells | cross tests | matched | V cross reads | V K-Q3 reads | M K-Q3 reads |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | 4096 | 16384 | 206 | 16384 | 0 | 15310 |
| 2 | 4096 | 16384 | 188 | 16384 | 0 | 15516 |
| 3 | 4096 | 16384 | 361 | 16384 | 0 | 15154 |
| 4 | 4096 | 16384 | 0 | 16384 | 0 | 14272 |
| 5 | 32768 | 131072 | 1564 | 131072 | 0 | 122471 |
| 6 | 32768 | 131072 | 1595 | 131072 | 0 | 122483 |
| 7 | 32768 | 131072 | 1362 | 131072 | 0 | 124311 |
| 8 | 32768 | 131072 | 1756 | 131072 | 0 | 121939 |
| 9 | 262144 | 1048576 | 12558 | 1048576 | 0 | 981601 |
| 10 | 262144 | 1048576 | 12783 | 1048576 | 0 | 976522 |
| 11 | 262144 | 1048576 | 12442 | 1048576 | 0 | 982162 |
| 12 | 262144 | 1048576 | 13668 | 1048576 | 0 | 961110 |
| 13 | 2097152 | 8388608 | 99227 | 8388480 | 0 | 7852405 |
| 14 | 2097152 | 8388608 | 102423 | 8388224 | 0 | 7831540 |
| 15 | 2097152 | 8388608 | 96562 | 8388608 | 0 | 7879804 |
| 16 | 2097152 | 8388608 | 96748 | 8388608 | 0 | 7802255 |

### Bytes per cell (accounted, at the end; tau included)

| job | TC | Hash-V-fixed | Hash-M-fixed | Hash-V-grow | Hash-M-grow | Hash-V-grow-map |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | 16.00 | 36.00 | 38.67 | 36.00 | 38.67 | 36.00 |
| 2 | 16.00 | 36.00 | 38.67 | 20.00 | 20.67 | 20.00 |
| 3 | 16.00 | 36.00 | 38.67 | 16.00 | 16.17 | 16.00 |
| 4 | 16.00 | 36.00 | 38.67 | 15.17 | 15.25 | 15.17 |
| 5 | 16.00 | 36.00 | 38.67 | 25.33 | 26.67 | 25.33 |
| 6 | 16.00 | 36.00 | 38.67 | 20.00 | 20.67 | 20.00 |
| 7 | 16.00 | 36.00 | 38.67 | 16.00 | 16.17 | 16.00 |
| 8 | 16.00 | 36.00 | 38.67 | 15.00 | 15.04 | 15.00 |
| 9 | 16.00 | 36.00 | 38.67 | 25.33 | 26.67 | 25.33 |
| 10 | 16.00 | 36.00 | 38.67 | 17.33 | 17.67 | 17.33 |
| 11 | 16.00 | 36.00 | 38.67 | 16.00 | 16.17 | 16.00 |
| 12 | 16.00 | 36.00 | 38.67 | 15.00 | 15.04 | 15.00 |
| 13 | 16.00 | 40.00 | 45.33 | 29.33 | 32.00 | 29.33 |
| 14 | 16.00 | 40.00 | 45.33 | 21.33 | 22.00 | 21.33 |
| 15 | 16.00 | 40.00 | 45.33 | 19.33 | 19.50 | 19.33 |
| 16 | 16.00 | 40.00 | 45.33 | 19.00 | 19.08 | 19.00 |

The gates checked every counter and store figure against the model (spec section 10 item 2), the TC gate included: MHVF:1 pass, TC:1 pass, MHMF:1 pass, MHVG:1 pass, MHMG:1 pass, MHVGM:1 pass, TC:2 pass, MHVF:2 pass, MHMF:2 pass, MHMG:2 pass, MHVG:2 pass, MHVGM:2 pass, MHVF:3 pass, TC:3 pass, MHMF:3 pass, MHMG:3 pass, MHVG:3 pass, MHVGM:3 pass, TC:4 pass, MHVF:4 pass, MHVG:4 pass, MHMF:4 pass, MHMG:4 pass, MHVGM:4 pass, MHVF:5 pass, TC:5 pass, MHMF:5 pass, MHVG:5 pass, MHMG:5 pass, MHVGM:5 pass, MHVF:6 pass, MHMF:6 pass, TC:6 pass, MHVG:6 pass, MHMG:6 pass, MHVGM:6 pass, MHVF:7 pass, TC:7 pass, MHMF:7 pass, MHVG:7 pass, MHMG:7 pass, MHVGM:7 pass, MHVF:8 pass, TC:8 pass, MHVG:8 pass, MHMF:8 pass, MHMG:8 pass, MHVGM:8 pass, MHVF:9 pass, TC:9 pass, MHMF:9 pass, MHVG:9 pass, MHMG:9 pass, MHVGM:9 pass, MHVF:10 pass, TC:10 pass, MHMF:10 pass, MHVG:10 pass, MHVGM:10 pass, MHMG:10 pass, MHVF:11 pass, MHMF:11 pass, TC:11 pass, MHVG:11 pass, MHMG:11 pass, MHVGM:11 pass, MHVF:12 pass, TC:12 pass, MHMF:12 pass, MHMG:12 pass, MHVG:12 pass, MHVGM:12 pass, MHVF:13 pass, TC:13 pass, MHMF:13 pass, MHVG:13 pass, MHMG:13 pass, MHVGM:13 pass, MHVF:14 pass, TC:14 pass, MHMF:14 pass, MHVG:14 pass, MHMG:14 pass, MHVGM:14 pass, MHVF:15 pass, TC:15 pass, MHMF:15 pass, MHVG:15 pass, MHMG:15 pass, MHVGM:15 pass, MHVF:16 pass, MHMF:16 pass, TC:16 pass, MHVG:16 pass, MHMG:16 pass, MHVGM:16 pass

### The idle and disturbance checks

As round 3 (rule-m1 item 5, carried over in full): processes are told apart by process, not by thread; the fork counter is reported only; host ticks are given as ticks / allowance (none in the pilot). The runner's own ticks are reported in total and per launch; the one sanity limit is more than 1 tick per launch plus 2 over a block.

| job | block | attempt | seconds | disturbed | runner ticks | spawns | runner ticks per launch | forks (reported) | forks less spawns (reported) | new processes under 1 tick (reported) | other limit | credited | host ticks |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | pilot | 1 | 3.716 | no | 3 | 20 | 0.150 | 20 | 0 | none | 9 | none | process_api 0 (6->6 threads), sh 0 (1->1 threads), environment-man 0 (9->9 threads), claude 0 (8->8 threads), bash 0 (1->1 threads) |
| 6 | pilot | 1 | 3.686 | no | 2 | 20 | 0.100 | 20 | 0 | none | 9 | none | process_api 0 (6->6 threads), sh 0 (1->1 threads), environment-man 0 (9->9 threads), claude 0 (8->7 threads), bash 0 (1->1 threads) |
| 13 | pilot | 1 | 13.403 | no | 2 | 20 | 0.100 | 20 | 0 | none | 19 | none | process_api 0 (6->6 threads), sh 0 (1->1 threads), environment-man 0 (9->9 threads), claude 1 (7->7 threads), bash 0 (1->1 threads) |
| 1 | measured | 1 | 18.292 | no | 13 | 90 | 0.144 | 92 | 2 | none | 24 | none | process_api 0/8 (6->6 threads), sh 0/8 (1->1 threads), environment-man 0/8 (9->9 threads), claude 2/8 (6->7 threads), bash 0/8 (1->1 threads) |
| 2 | measured | 1 | 17.772 | no | 14 | 90 | 0.156 | 91 | 1 | none | 23 | none | process_api 1/8 (6->6 threads), sh 0/8 (1->1 threads), environment-man 2/8 (9->9 threads), claude 2/8 (7->8 threads), bash 0/8 (1->1 threads) |
| 3 | measured | 1 | 17.876 | no | 14 | 90 | 0.156 | 90 | 0 | none | 23 | none | process_api 0/8 (6->6 threads), sh 0/8 (1->1 threads), environment-man 0/8 (9->9 threads), claude 2/8 (8->7 threads), bash 0/8 (1->1 threads) |
| 4 | measured | 1 | 17.8 | no | 13 | 90 | 0.144 | 92 | 2 | none | 23 | none | process_api 1/8 (6->6 threads), sh 0/8 (1->1 threads), environment-man 0/8 (9->9 threads), claude 2/8 (6->7 threads), bash 0/8 (1->1 threads) |
| 5 | measured | 1 | 18.279 | no | 14 | 90 | 0.156 | 91 | 1 | none | 24 | none | process_api 1/8 (6->6 threads), sh 0/8 (1->1 threads), environment-man 0/8 (9->9 threads), claude 2/8 (7->8 threads), bash 0/8 (1->1 threads) |
| 6 | measured | 1 | 18.172 | no | 14 | 90 | 0.156 | 90 | 0 | none | 24 | none | process_api 0/8 (6->6 threads), sh 0/8 (1->1 threads), environment-man 0/8 (9->9 threads), claude 1/8 (8->7 threads), bash 0/8 (1->1 threads) |
| 7 | measured | 1 | 18.321 | no | 14 | 90 | 0.156 | 92 | 2 | none | 24 | none | process_api 0/8 (6->6 threads), sh 0/8 (1->1 threads), environment-man 1/8 (9->9 threads), claude 2/8 (6->7 threads), bash 0/8 (1->1 threads) |
| 8 | measured | 1 | 17.679 | no | 14 | 90 | 0.156 | 91 | 1 | none | 23 | none | process_api 1/8 (6->6 threads), sh 0/8 (1->1 threads), environment-man 1/8 (9->9 threads), claude 2/8 (7->8 threads), bash 0/8 (1->1 threads) |
| 9 | measured | 1 | 18.531 | no | 13 | 90 | 0.144 | 90 | 0 | none | 24 | none | process_api 0/8 (6->6 threads), sh 0/8 (1->1 threads), environment-man 0/8 (9->9 threads), claude 3/8 (8->6 threads), bash 0/8 (1->1 threads) |
| 10 | measured | 1 | 18.156 | no | 13 | 90 | 0.144 | 92 | 2 | none | 24 | none | process_api 1/8 (6->6 threads), sh 0/8 (1->1 threads), environment-man 1/8 (9->9 threads), claude 2/8 (6->7 threads), bash 0/8 (1->1 threads) |
| 11 | measured | 1 | 18.381 | no | 13 | 90 | 0.144 | 92 | 2 | none | 24 | none | process_api 1/8 (6->6 threads), sh 0/8 (1->1 threads), environment-man 0/8 (9->9 threads), claude 3/8 (7->8 threads), bash 0/8 (1->1 threads) |
| 12 | measured | 1 | 17.864 | no | 12 | 90 | 0.133 | 90 | 0 | none | 23 | none | process_api 0/8 (6->6 threads), sh 0/8 (1->1 threads), environment-man 0/8 (9->9 threads), claude 2/8 (8->6 threads), bash 0/8 (1->1 threads) |
| 13 | measured | 1 | 72.793 | no | 15 | 90 | 0.167 | 96 | 6 | none | 78 | none | process_api 2/16 (6->6 threads), sh 0/16 (1->1 threads), environment-man 1/16 (9->9 threads), claude 8/16 (6->8 threads), bash 0/16 (1->1 threads) |
| 14 | measured | 1 | 72.197 | no | 14 | 90 | 0.156 | 96 | 6 | none | 78 | none | process_api 2/16 (6->6 threads), sh 0/16 (1->1 threads), environment-man 2/16 (9->9 threads), claude 8/16 (7->8 threads), bash 0/16 (1->1 threads) |
| 15 | measured | 1 | 72.041 | no | 14 | 90 | 0.156 | 97 | 7 | none | 78 | none | process_api 2/16 (6->6 threads), sh 0/16 (1->1 threads), environment-man 1/16 (9->9 threads), claude 7/16 (8->8 threads), bash 0/16 (1->1 threads) |
| 16 | measured | 1 | 71.867 | yes: host process claude[93] used 23 ticks, over its allowance 16 | 13 | 90 | 0.144 | 96 | 6 | none | 77 | none | process_api 2/16 (6->6 threads), sh 0/16 (1->1 threads), environment-man 2/16 (9->9 threads), claude 23/16 (8->7 threads), bash 0/16 (1->1 threads) |
| 16 | measured | 2 | 72.063 | no | 14 | 90 | 0.156 | 96 | 6 | none | 78 | none | process_api 2/16 (6->6 threads), sh 0/16 (1->1 threads), environment-man 1/16 (9->9 threads), claude 10/16 (6->7 threads), bash 0/16 (1->1 threads) |

The runner's own ticks over the 20 blocks: 238 ticks over 1590 launches, 0.150 per launch; the highest block 0.167 per launch (sanity limit: more than 1 per launch plus 2 per block). Reported only: they fall between reps, not in them.

### The measured process's own CPU time (reported only; enters no verdict)

CPU share: the timed process's CPU time (CLOCK_PROCESS_CPUTIME_ID) over the CLOCK_MONOTONIC time of the same whole rep, for the rep kept in each round.

| job | row | rounds | CPU share, median | CPU share, lowest | CPU ns per run, median | total ns per run, median |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | TC | 12 | 0.9995 | 0.9827 | 440933 | 441377 |
| 1 | Hash-V-fixed | 12 | 1.0000 | 0.9782 | 506887 | 506771 |
| 1 | Hash-M-fixed | 12 | 0.9998 | 0.9872 | 518984 | 519318 |
| 1 | Hash-V-grow | 12 | 0.9998 | 0.9750 | 552024 | 551923 |
| 1 | Hash-M-grow | 12 | 0.9998 | 0.9917 | 565630 | 566128 |
| 1 | Hash-V-grow-map | 12 | 0.9999 | 0.9830 | 540004 | 539993 |
| 1 | Hash-V-fixed-null | 12 | 0.9986 | 0.9824 | 506796 | 507568 |
| 2 | TC | 12 | 0.9997 | 0.9942 | 435151 | 435054 |
| 2 | Hash-V-fixed | 12 | 0.9988 | 0.9911 | 503674 | 503627 |
| 2 | Hash-M-fixed | 12 | 0.9988 | 0.9866 | 510703 | 511962 |
| 2 | Hash-V-grow | 12 | 0.9995 | 0.9852 | 524680 | 525401 |
| 2 | Hash-M-grow | 12 | 0.9999 | 0.9880 | 538178 | 539114 |
| 2 | Hash-V-grow-map | 12 | 0.9995 | 0.9848 | 522644 | 522515 |
| 2 | Hash-V-fixed-null | 12 | 0.9994 | 0.9816 | 503815 | 503597 |
| 3 | TC | 12 | 0.9996 | 0.9954 | 381796 | 382058 |
| 3 | Hash-V-fixed | 12 | 0.9954 | 0.9760 | 456943 | 459271 |
| 3 | Hash-M-fixed | 12 | 0.9997 | 0.9871 | 458444 | 458447 |
| 3 | Hash-V-grow | 12 | 0.9980 | 0.9851 | 470943 | 473567 |
| 3 | Hash-M-grow | 12 | 0.9999 | 0.9850 | 475883 | 476425 |
| 3 | Hash-V-grow-map | 12 | 0.9996 | 0.9937 | 468533 | 469175 |
| 3 | Hash-V-fixed-null | 12 | 0.9996 | 0.9791 | 456864 | 456843 |
| 4 | TC | 12 | 0.9985 | 0.9740 | 380060 | 380410 |
| 4 | Hash-V-fixed | 12 | 0.9997 | 0.8525 | 451529 | 451306 |
| 4 | Hash-M-fixed | 12 | 0.9995 | 0.9893 | 454076 | 454590 |
| 4 | Hash-V-grow | 12 | 0.9998 | 0.9631 | 467953 | 468079 |
| 4 | Hash-M-grow | 12 | 0.9996 | 0.9900 | 471016 | 471258 |
| 4 | Hash-V-grow-map | 12 | 0.9999 | 0.9890 | 463089 | 462834 |
| 4 | Hash-V-fixed-null | 12 | 0.9998 | 0.9823 | 451810 | 452045 |
| 5 | TC | 12 | 0.9994 | 0.9897 | 3738556 | 3741143 |
| 5 | Hash-V-fixed | 12 | 0.9998 | 0.9949 | 4199318 | 4199864 |
| 5 | Hash-M-fixed | 12 | 0.9996 | 0.9933 | 4232143 | 4235182 |
| 5 | Hash-V-grow | 12 | 1.0000 | 0.9958 | 4329411 | 4329114 |
| 5 | Hash-M-grow | 12 | 0.9986 | 0.9954 | 4386628 | 4392752 |
| 5 | Hash-V-grow-map | 12 | 0.9995 | 0.9901 | 4280154 | 4286944 |
| 5 | Hash-V-fixed-null | 12 | 0.9998 | 0.9977 | 4192426 | 4193075 |
| 6 | TC | 12 | 0.9991 | 0.9903 | 3720560 | 3728853 |
| 6 | Hash-V-fixed | 12 | 0.9994 | 0.9874 | 4180501 | 4180177 |
| 6 | Hash-M-fixed | 12 | 0.9990 | 0.9853 | 4196510 | 4203356 |
| 6 | Hash-V-grow | 12 | 0.9994 | 0.9791 | 4278488 | 4283380 |
| 6 | Hash-M-grow | 12 | 0.9996 | 0.9943 | 4330541 | 4335904 |
| 6 | Hash-V-grow-map | 12 | 0.9997 | 0.9968 | 4246818 | 4250878 |
| 6 | Hash-V-fixed-null | 12 | 0.9987 | 0.9838 | 4181136 | 4188089 |
| 7 | TC | 12 | 0.9989 | 0.9676 | 3733072 | 3736716 |
| 7 | Hash-V-fixed | 12 | 0.9973 | 0.9833 | 4184282 | 4197428 |
| 7 | Hash-M-fixed | 12 | 0.9990 | 0.9765 | 4202989 | 4214611 |
| 7 | Hash-V-grow | 12 | 0.9989 | 0.9852 | 4269293 | 4290775 |
| 7 | Hash-M-grow | 12 | 0.9997 | 0.9940 | 4305176 | 4311029 |
| 7 | Hash-V-grow-map | 12 | 0.9999 | 0.8796 | 4242344 | 4244285 |
| 7 | Hash-V-fixed-null | 12 | 0.9978 | 0.9853 | 4196305 | 4204831 |
| 8 | TC | 12 | 0.9986 | 0.9967 | 3565722 | 3566381 |
| 8 | Hash-V-fixed | 12 | 0.9994 | 0.9720 | 4048149 | 4057052 |
| 8 | Hash-M-fixed | 12 | 0.9990 | 0.9915 | 4060822 | 4067816 |
| 8 | Hash-V-grow | 12 | 0.9977 | 0.9777 | 4131846 | 4134420 |
| 8 | Hash-M-grow | 12 | 0.9990 | 0.9866 | 4163749 | 4165825 |
| 8 | Hash-V-grow-map | 12 | 0.9989 | 0.9717 | 4107471 | 4115534 |
| 8 | Hash-V-fixed-null | 12 | 0.9980 | 0.9920 | 4045141 | 4057424 |
| 9 | TC | 12 | 0.9995 | 0.9881 | 30623339 | 30666032 |
| 9 | Hash-V-fixed | 12 | 0.9993 | 0.9526 | 34092970 | 34125046 |
| 9 | Hash-M-fixed | 12 | 0.9991 | 0.9858 | 34346607 | 34427470 |
| 9 | Hash-V-grow | 12 | 0.9987 | 0.9680 | 34684361 | 34720290 |
| 9 | Hash-M-grow | 12 | 0.9994 | 0.9875 | 35135554 | 35178696 |
| 9 | Hash-V-grow-map | 12 | 0.9986 | 0.9833 | 34525911 | 34617740 |
| 9 | Hash-V-fixed-null | 12 | 0.9987 | 0.9834 | 34125836 | 34158136 |
| 10 | TC | 12 | 0.9999 | 0.9915 | 30470701 | 30485438 |
| 10 | Hash-V-fixed | 12 | 0.9994 | 0.9970 | 33827614 | 33870823 |
| 10 | Hash-M-fixed | 12 | 0.9999 | 0.9962 | 33963304 | 33969272 |
| 10 | Hash-V-grow | 12 | 0.9999 | 0.9958 | 34202059 | 34200743 |
| 10 | Hash-M-grow | 12 | 0.9998 | 0.9919 | 34640124 | 34691065 |
| 10 | Hash-V-grow-map | 12 | 0.9999 | 0.9637 | 34120126 | 34121573 |
| 10 | Hash-V-fixed-null | 12 | 0.9994 | 0.9915 | 33905559 | 33918088 |
| 11 | TC | 12 | 0.9996 | 0.9525 | 30578602 | 30583583 |
| 11 | Hash-V-fixed | 12 | 0.9996 | 0.9932 | 33932404 | 33967409 |
| 11 | Hash-M-fixed | 12 | 0.9994 | 0.9956 | 34110612 | 34131556 |
| 11 | Hash-V-grow | 12 | 0.9992 | 0.9764 | 34331576 | 34370575 |
| 11 | Hash-M-grow | 12 | 0.9997 | 0.9866 | 34666428 | 34698763 |
| 11 | Hash-V-grow-map | 12 | 0.9995 | 0.9766 | 34234832 | 34268386 |
| 11 | Hash-V-fixed-null | 12 | 1.0000 | 0.9861 | 33906672 | 33905640 |
| 12 | TC | 12 | 0.9992 | 0.9857 | 29743044 | 29802833 |
| 12 | Hash-V-fixed | 12 | 0.9999 | 0.9946 | 33207591 | 33285706 |
| 12 | Hash-M-fixed | 12 | 0.9998 | 0.9868 | 33279629 | 33360570 |
| 12 | Hash-V-grow | 12 | 0.9993 | 0.9861 | 33640161 | 33700100 |
| 12 | Hash-M-grow | 12 | 0.9994 | 0.9890 | 33862228 | 33874199 |
| 12 | Hash-V-grow-map | 12 | 0.9995 | 0.9842 | 33442818 | 33469840 |
| 12 | Hash-V-fixed-null | 12 | 0.9995 | 0.9959 | 33131816 | 33153568 |
| 13 | TC | 12 | 0.9994 | 0.9855 | 249354833 | 249962881 |
| 13 | Hash-V-fixed | 12 | 0.9994 | 0.9871 | 276220636 | 276536604 |
| 13 | Hash-M-fixed | 12 | 0.9971 | 0.9861 | 281812776 | 282916064 |
| 13 | Hash-V-grow | 12 | 0.9982 | 0.8892 | 277295472 | 277807908 |
| 13 | Hash-M-grow | 12 | 0.9990 | 0.9568 | 282445512 | 282535018 |
| 13 | Hash-V-grow-map | 12 | 0.9975 | 0.9894 | 277069810 | 277925794 |
| 13 | Hash-V-fixed-null | 12 | 0.9989 | 0.9855 | 276367608 | 278258914 |
| 14 | TC | 12 | 0.9954 | 0.9905 | 247079393 | 248403544 |
| 14 | Hash-V-fixed | 12 | 0.9966 | 0.9879 | 274825933 | 275776474 |
| 14 | Hash-M-fixed | 12 | 0.9968 | 0.9652 | 278896288 | 279713174 |
| 14 | Hash-V-grow | 12 | 0.9976 | 0.9867 | 274149034 | 276170590 |
| 14 | Hash-M-grow | 12 | 0.9984 | 0.9807 | 277257334 | 277687163 |
| 14 | Hash-V-grow-map | 12 | 0.9970 | 0.9733 | 274099096 | 275720642 |
| 14 | Hash-V-fixed-null | 12 | 0.9979 | 0.9801 | 273974946 | 275246494 |
| 15 | TC | 12 | 0.9990 | 0.9921 | 248673460 | 248970981 |
| 15 | Hash-V-fixed | 12 | 0.9992 | 0.9836 | 274721428 | 275806392 |
| 15 | Hash-M-fixed | 12 | 0.9995 | 0.9940 | 278732080 | 279000730 |
| 15 | Hash-V-grow | 12 | 0.9987 | 0.9700 | 274125672 | 274177806 |
| 15 | Hash-M-grow | 12 | 0.9971 | 0.9863 | 277567964 | 278487960 |
| 15 | Hash-V-grow-map | 12 | 0.9991 | 0.9940 | 274907012 | 275084839 |
| 15 | Hash-V-fixed-null | 12 | 0.9978 | 0.9650 | 274803170 | 275934929 |
| 16 | TC | 12 | 0.9930 | 0.9503 | 247315453 | 249376561 |
| 16 | Hash-V-fixed | 12 | 0.9995 | 0.9941 | 273261982 | 273295067 |
| 16 | Hash-M-fixed | 12 | 0.9966 | 0.9931 | 277216584 | 277819306 |
| 16 | Hash-V-grow | 12 | 0.9975 | 0.9809 | 275322631 | 277755758 |
| 16 | Hash-M-grow | 12 | 0.9979 | 0.9916 | 276001082 | 277025914 |
| 16 | Hash-V-grow-map | 12 | 0.9973 | 0.9795 | 274527731 | 275473492 |
| 16 | Hash-V-fixed-null | 12 | 0.9960 | 0.9662 | 273754004 | 275819982 |

Idle checks: 20 (0 busy). Host rates R_h: {'1': 0.07461016190405133, '79': 0.07461016190405133, '85': 0.07461016190405133, '93': 0.07461290259634619, '16282': 0.07461016190405133} (measured: {'1': 0.0, '79': 0.0, '85': 0.0, '93': 0.07461290259634619, '16282': 0.0}; R_h is at least 1 tick over the longest pilot block).

### Round 3, for reference only

Round 3's results (Design 3 on streams of single operations) are in [`../../../10-interning/test/results-r3/results.md`](../../../10-interning/test/results-r3/results.md). They enter no measure here.

The addition run's report, for reference only: [`../results-m1-add/results.md`](../results-m1-add/results.md). Nothing measured there enters a measure here.

## Limitations

- **Textbook only.** No row is optimized: plain open addressing with linear probing; growth by doubling.
- **The index store is not tested** in this round (the author's decision).
- **Fixed's fill is low and not controlled:** its tables are sized from the dimensions (T >= 2n^2 + 2), and it loses on memory by design. **Grow has a fill limit** of one half (spec section 5.6), which the author rejected for round 3's fixed table.
- **The map method is tested on V only;** the rewrite method and V/M are taken to act on different parts (creation and operation), though C's growths put a little of the rewrite into the operation part.
- **Addition is floor-dominated:** its runs take milliseconds, so its time ratios rest on reps of about 50 ms, made of many runs; the null band shows how far that holds.
- **Two runs, not one:** the runs are separate in time, so machine conditions can differ between them; their results are not combined.
- **64 bits only.** A value past 64 bits is an error in every build; the sweep stays far below.
- **Square matrices, one value distribution.** A and B have the same sharing level.
- Every other limitation of round 3 that applies (the disturbance check cannot see every process; a child of a process that ignores SIGCHLD is not charged).

