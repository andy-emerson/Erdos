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
