# Design 3 on matrix addition: results

Run started 2026-10-30T00:36:30-05:00.

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

- The addition run (job set `add`).
- Started 2026-10-30T00:36:30-05:00; finished 2026-10-30T00:43:17-05:00.
- Step 0 (before the clock): 2.1 s.
- Run time 6.7 minutes (hard stop 40).
- Rule sha256 `8e951ac6597f099962aa77580653e799d101af6d7b3e30a51e1a6005ab7aeff7`; round 3's results after the run: unchanged.
- Host processes (the runner's ancestors): bash[14116], claude[93], environment-man[85], sh[79], process_api[1]
- Code at 2026-10-30T00:36:33-05:00: TC `80e68597f321`, MHVF `86905b11a897`, MHMF `76c706ad7a52`, MHVG `0624617cc9f3`, MHMG `b0703662a2f2`, MHVGM `260bbcf2d702`, TC3GATE `1faeb759cc4c`
- Unit tests: m1_unit: m1_unit: 4014 checks, 0 failures; m1_diff: m1_diff: 3000 seeds, 3000 jobs, 747676 operations; 1591644 checks, 0 failures; m1_alloc-timing: m1_alloc (timing build): 180 jobs, 18312 checks, 0 failures; m1_alloc-counting: m1_alloc (counting build): 180 jobs, 18312 checks, 0 failures; m1_unit-asan: m1_unit: 4014 checks, 0 failures; m1_diff-asan: m1_diff: 600 seeds, 600 jobs, 151723 operations; 324084 checks, 0 failures

## The pilot (discarded, except what it records for the run)

- Jobs [1, 6, 13]; block wall times job 1 5.7 s, job 6 5.8 s, job 13 5.8 s; P_6 = 5.8 s, P_13 = 5.8 s; N_128 = 4, N_rest = 12.
- Projection elapsed + 1.2 x 5 x (N_128 P_13 + N_rest P_6) = 10.0 min (limit 30).
- Host rates R_h (ticks per second): {'1': 0.26198585276395076, '79': 0.26198585276395076, '85': 0.26198585276395076, '93': 0.26890781098081756, '14116': 0.26198585276395076}.

## Verdicts, per job, pairing and measure (no overall winner)

Time: T(reference)/T(row), above 1 the row is faster; creation and operation each have their own band h from Hash-V-fixed against Hash-V-fixed-null on that part. Total and read-out time are reported without a verdict. Bytes per cell: row over reference, below 1 the row is smaller (end of the operation; after creation in parentheses). Map against lookup is judged on creation only. Gcd work is a check, not a verdict. (c): the other rows' own noise is not measured: the null row is a copy of Hash-V-fixed, and on this job a row of the pairing ran 1 rep per invocation (its calibration rep lasted at least 1 s), so its noise may differ (rule-m1 item 8).

### Hash-V-fixed against TC (against TC)

| job | creation: verdict, ratio [lo, hi], h | operation: verdict, ratio [lo, hi], h | total (reported) | read-out (reported) | B_used (end): verdict, ratio (after creation) | B_heap (end): verdict, ratio (after creation) |
| --- | --- | --- | --- | --- | --- | --- |
| 1 (n 16, rho 1) | loss, 0.563 [0.555, 0.574], h 0.050 | loss, 0.450 [0.442, 0.456], h 0.050 | reported, 0.514 [0.506, 0.522] | reported, 0.631 [0.605, 0.791] | loss, 1.917 (1.667) | loss, 1.990 (1.663) |
| 2 (n 16, rho 4) | loss, 0.602 [0.599, 0.613], h 0.050 | loss, 0.426 [0.422, 0.428], h 0.050 | reported, 0.519 [0.514, 0.523] | reported, 0.616 [0.610, 0.795] | loss, 1.917 (1.667) | loss, 1.990 (1.663) |
| 3 (n 16, rho 16) | loss, 0.612 [0.600, 0.619], h 0.053 | loss, 0.477 [0.471, 0.479], h 0.053 | reported, 0.546 [0.539, 0.553] | reported, 0.624 [0.615, 0.714] | loss, 1.917 (1.667) | loss, 1.990 (1.663) |
| 4 (n 16, rho 64) | loss, 0.613 [0.603, 0.621], h 0.050 | loss, 0.534 [0.530, 0.543], h 0.050 | reported, 0.576 [0.571, 0.586] | reported, 0.630 [0.611, 0.791] | loss, 1.917 (1.667) | loss, 1.990 (1.663) |
| 5 (n 32, rho 1) | loss, 0.595 [0.587, 0.615], h 0.050 | loss, 0.470 [0.461, 0.478], h 0.050 | reported, 0.542 [0.535, 0.556] | reported, 0.633 [0.610, 0.745] | loss, 1.917 (1.667) | loss, 1.914 (1.666) |
| 6 (n 32, rho 4) | loss, 0.580 [0.572, 0.597], h 0.050 | loss, 0.484 [0.481, 0.492], h 0.050 | reported, 0.545 [0.539, 0.552] | reported, 0.657 [0.626, 0.850] | loss, 1.917 (1.667) | loss, 1.914 (1.666) |
| 7 (n 32, rho 16) | loss, 0.607 [0.593, 0.616], h 0.050 | loss, 0.513 [0.500, 0.518], h 0.050 | reported, 0.572 [0.555, 0.577] | reported, 0.689 [0.636, 0.853] | loss, 1.917 (1.667) | loss, 1.914 (1.666) |
| 8 (n 32, rho 64) | loss, 0.516 [0.507, 0.522], h 0.050 | loss, 0.390 [0.385, 0.394], h 0.050 | reported, 0.465 [0.462, 0.472] | reported, 0.700 [0.634, 0.848] | loss, 1.917 (1.667) | loss, 1.914 (1.666) |
| 9 (n 64, rho 1) | loss, 0.642 [0.638, 0.645], h 0.050 | loss, 0.581 [0.579, 0.583], h 0.050 | reported, 0.619 [0.617, 0.623] | reported, 0.794 [0.667, 0.895] | loss, 1.917 (1.667) | loss, 1.916 (1.666) |
| 10 (n 64, rho 4) | loss, 0.653 [0.648, 0.655], h 0.050 | loss, 0.583 [0.580, 0.587], h 0.050 | reported, 0.625 [0.623, 0.627] | reported, 0.655 [0.649, 0.671] | loss, 1.917 (1.667) | loss, 1.916 (1.666) |
| 11 (n 64, rho 16) | loss, 0.677 [0.672, 0.683], h 0.050 | loss, 0.575 [0.568, 0.581], h 0.050 | reported, 0.637 [0.629, 0.641] | reported, 0.698 [0.659, 0.804] | loss, 1.917 (1.667) | loss, 1.916 (1.666) |
| 12 (n 64, rho 64) | loss, 0.684 [0.680, 0.691], h 0.050 | loss, 0.553 [0.547, 0.570], h 0.050 | reported, 0.626 [0.621, 0.636] | reported, 0.687 [0.664, 0.723] | loss, 1.917 (1.667) | loss, 1.916 (1.666) |
| 13 (n 128, rho 1) | loss, 0.643 [0.640, 0.647], h 0.050 | loss, 0.568 [0.562, 0.572], h 0.050 | reported, 0.613 [0.611, 0.616] | reported, 0.625 [0.613, 0.641] | loss, 2.167 (1.917) **severe memory loss** | loss, 2.166 (1.917) **severe memory loss** |
| 14 (n 128, rho 4) | loss, 0.674 [0.667, 0.677], h 0.050 | loss, 0.590 [0.585, 0.597], h 0.050 | reported, 0.639 [0.634, 0.646] | reported, 0.679 [0.632, 0.830] | loss, 2.167 (1.917) **severe memory loss** | loss, 2.166 (1.917) **severe memory loss** |
| 15 (n 128, rho 16) | loss, 0.706 [0.696, 0.712], h 0.050 | loss, 0.576 [0.568, 0.581], h 0.050 | reported, 0.653 [0.641, 0.658] | reported, 0.707 [0.626, 0.844] | loss, 2.167 (1.917) **severe memory loss** | loss, 2.166 (1.917) **severe memory loss** |
| 16 (n 128, rho 64) | loss, 0.707 [0.703, 0.710], h 0.050 | loss, 0.573 [0.571, 0.578], h 0.050 | reported, 0.651 [0.649, 0.655] | reported, 0.663 [0.643, 0.670] | loss, 2.167 (1.917) **severe memory loss** | loss, 2.166 (1.917) **severe memory loss** |

### Hash-M-fixed against TC (against TC)

| job | creation: verdict, ratio [lo, hi], h | operation: verdict, ratio [lo, hi], h | total (reported) | read-out (reported) | B_used (end): verdict, ratio (after creation) | B_heap (end): verdict, ratio (after creation) |
| --- | --- | --- | --- | --- | --- | --- |
| 1 (n 16, rho 1) | loss, 0.547 [0.540, 0.555], h 0.050 | loss, 0.330 [0.323, 0.336], h 0.050 | reported, 0.434 [0.425, 0.442] | reported, 0.620 [0.595, 0.788] | loss, 2.083 (1.667) **severe memory loss** | loss, 2.158 (1.667) **severe memory loss** |
| 2 (n 16, rho 4) | loss, 0.600 [0.595, 0.608], h 0.050 | loss, 0.345 [0.341, 0.349], h 0.050 | reported, 0.465 [0.459, 0.471] | reported, 0.610 [0.598, 0.787] | loss, 2.083 (1.667) **severe memory loss** | loss, 2.158 (1.667) **severe memory loss** |
| 3 (n 16, rho 16) | loss, 0.617 [0.612, 0.622], h 0.053 | loss, 0.414 [0.412, 0.416], h 0.053 | reported, 0.510 [0.506, 0.517] | reported, 0.615 [0.608, 0.705] | loss, 2.083 (1.667) **severe memory loss** | loss, 2.158 (1.667) **severe memory loss** |
| 4 (n 16, rho 64) | loss, 0.619 [0.611, 0.626], h 0.050 | loss, 0.477 [0.474, 0.486], h 0.050 | reported, 0.547 [0.544, 0.556] | reported, 0.625 [0.607, 0.782] | loss, 2.083 (1.667) **severe memory loss** | loss, 2.158 (1.667) **severe memory loss** |
| 5 (n 32, rho 1) | loss, 0.596 [0.590, 0.612], h 0.050 | loss, 0.354 [0.348, 0.359], h 0.050 | reported, 0.477 [0.474, 0.487] | reported, 0.631 [0.610, 0.743] | loss, 2.083 (1.667) **severe memory loss** | loss, 2.081 (1.667) **severe memory loss** |
| 6 (n 32, rho 4) | loss, 0.601 [0.597, 0.618], h 0.050 | loss, 0.413 [0.410, 0.420], h 0.050 | reported, 0.518 [0.514, 0.525] | reported, 0.650 [0.619, 0.833] | loss, 2.083 (1.667) **severe memory loss** | loss, 2.081 (1.667) **severe memory loss** |
| 7 (n 32, rho 16) | loss, 0.668 [0.659, 0.675], h 0.050 | loss, 0.461 [0.452, 0.465], h 0.050 | reported, 0.573 [0.559, 0.576] | reported, 0.677 [0.616, 0.847] | loss, 2.083 (1.667) **severe memory loss** | loss, 2.081 (1.667) **severe memory loss** |
| 8 (n 32, rho 64) | loss, 0.579 [0.568, 0.586], h 0.050 | loss, 0.364 [0.359, 0.368], h 0.050 | reported, 0.478 [0.476, 0.486] | reported, 0.694 [0.628, 0.841] | loss, 2.083 (1.667) **severe memory loss** | loss, 2.081 (1.667) **severe memory loss** |
| 9 (n 64, rho 1) | loss, 0.670 [0.665, 0.673], h 0.050 | loss, 0.439 [0.437, 0.442], h 0.050 | reported, 0.560 [0.558, 0.564] | reported, 0.763 [0.637, 0.865] | loss, 2.083 (1.667) **severe memory loss** | loss, 2.083 (1.667) **severe memory loss** |
| 10 (n 64, rho 4) | loss, 0.692 [0.688, 0.695], h 0.050 | loss, 0.506 [0.504, 0.509], h 0.050 | reported, 0.609 [0.607, 0.610] | reported, 0.641 [0.628, 0.666] | loss, 2.083 (1.667) **severe memory loss** | loss, 2.083 (1.667) **severe memory loss** |
| 11 (n 64, rho 16) | loss, 0.716 [0.711, 0.723], h 0.050 | loss, 0.530 [0.524, 0.536], h 0.050 | reported, 0.633 [0.627, 0.638] | reported, 0.668 [0.630, 0.781] | loss, 2.083 (1.667) **severe memory loss** | loss, 2.083 (1.667) **severe memory loss** |
| 12 (n 64, rho 64) | loss, 0.718 [0.713, 0.724], h 0.050 | loss, 0.514 [0.509, 0.525], h 0.050 | reported, 0.620 [0.616, 0.629] | reported, 0.664 [0.642, 0.699] | loss, 2.083 (1.667) **severe memory loss** | loss, 2.083 (1.667) **severe memory loss** |
| 13 (n 128, rho 1) | loss, 0.673 [0.670, 0.676], h 0.050 | loss, 0.450 [0.443, 0.454], h 0.050 | reported, 0.569 [0.563, 0.573] | reported, 0.602 [0.593, 0.620] | loss, 2.500 (1.917) **severe memory loss** | loss, 2.500 (1.917) **severe memory loss** |
| 14 (n 128, rho 4) | loss, 0.713 [0.709, 0.716], h 0.050 | loss, 0.509 [0.500, 0.511], h 0.050 | reported, 0.620 [0.613, 0.623] | reported, 0.675 [0.635, 0.820] | loss, 2.500 (1.917) **severe memory loss** | loss, 2.500 (1.917) **severe memory loss** |
| 15 (n 128, rho 16) | loss, 0.748 [0.744, 0.754], h 0.050 | loss, 0.517 [0.514, 0.520], h 0.050 | reported, 0.641 [0.639, 0.648] | reported, 0.701 [0.631, 0.815] | loss, 2.500 (1.917) **severe memory loss** | loss, 2.500 (1.917) **severe memory loss** |
| 16 (n 128, rho 64) | loss, 0.750 [0.748, 0.754], h 0.050 | loss, 0.522 [0.516, 0.530], h 0.050 | reported, 0.645 [0.642, 0.650] | reported, 0.637 [0.609, 0.647] | loss, 2.500 (1.917) **severe memory loss** | loss, 2.500 (1.917) **severe memory loss** |

### Hash-V-grow against TC (against TC)

| job | creation: verdict, ratio [lo, hi], h | operation: verdict, ratio [lo, hi], h | total (reported) | read-out (reported) | B_used (end): verdict, ratio (after creation) | B_heap (end): verdict, ratio (after creation) |
| --- | --- | --- | --- | --- | --- | --- |
| 1 (n 16, rho 1) | loss, 0.165 [0.164, 0.167], h 0.050 | loss, 0.258 [0.254, 0.261], h 0.050 | reported, 0.202 [0.200, 0.204] | reported, 0.628 [0.605, 0.789] | loss, 1.917 (1.585) | loss, 2.089 (1.752) **severe memory loss** |
| 2 (n 16, rho 4) | loss, 0.381 [0.373, 0.387], h 0.050 | loss, 0.252 [0.249, 0.255], h 0.050 | reported, 0.321 [0.317, 0.325] | reported, 0.615 [0.602, 0.794] | tie, 0.917 (0.585) | loss, 1.112 (0.775) |
| 3 (n 16, rho 16) | loss, 0.499 [0.495, 0.505], h 0.053 | loss, 0.341 [0.338, 0.343], h 0.053 | reported, 0.419 [0.415, 0.425] | reported, 0.617 [0.604, 0.709] | win, 0.500 (0.335) | win, 0.701 (0.406) |
| 4 (n 16, rho 64) | loss, 0.526 [0.513, 0.533], h 0.050 | loss, 0.505 [0.499, 0.515], h 0.050 | reported, 0.520 [0.511, 0.529] | reported, 0.629 [0.602, 0.791] | win, 0.312 (0.293) | win, 0.372 (0.344) |
| 5 (n 32, rho 1) | loss, 0.299 [0.297, 0.308], h 0.050 | loss, 0.244 [0.240, 0.248], h 0.050 | reported, 0.280 [0.277, 0.285] | reported, 0.641 [0.619, 0.757] | loss, 1.250 (0.917) | loss, 1.298 (0.963) |
| 6 (n 32, rho 4) | loss, 0.354 [0.351, 0.363], h 0.050 | loss, 0.250 [0.247, 0.254], h 0.050 | reported, 0.311 [0.308, 0.315] | reported, 0.658 [0.621, 0.845] | tie, 0.917 (0.584) | tie, 0.966 (0.632) |
| 7 (n 32, rho 16) | loss, 0.505 [0.498, 0.511], h 0.050 | loss, 0.359 [0.352, 0.362], h 0.050 | reported, 0.440 [0.430, 0.444] | reported, 0.688 [0.631, 0.854] | win, 0.500 (0.334) | win, 0.551 (0.383) |
| 8 (n 32, rho 64) | loss, 0.449 [0.440, 0.453], h 0.050 | loss, 0.307 [0.301, 0.310], h 0.050 | reported, 0.388 [0.385, 0.393] | reported, 0.705 [0.640, 0.852] | win, 0.354 (0.271) | win, 0.406 (0.290) |
| 9 (n 64, rho 1) | loss, 0.374 [0.371, 0.375], h 0.050 | loss, 0.392 [0.390, 0.394], h 0.050 | reported, 0.383 [0.382, 0.385] | reported, 0.793 [0.665, 0.891] | tie, 1.083 (0.917) | tie, 1.095 (0.928) |
| 10 (n 64, rho 4) | loss, 0.529 [0.525, 0.531], h 0.050 | loss, 0.394 [0.392, 0.396], h 0.050 | reported, 0.471 [0.469, 0.472] | reported, 0.662 [0.656, 0.678] | win, 0.583 (0.417) | win, 0.596 (0.429) |
| 11 (n 64, rho 16) | loss, 0.593 [0.588, 0.598], h 0.050 | loss, 0.390 [0.386, 0.395], h 0.050 | reported, 0.498 [0.494, 0.501] | reported, 0.698 [0.658, 0.804] | win, 0.500 (0.333) | win, 0.513 (0.346) |
| 12 (n 64, rho 64) | loss, 0.643 [0.641, 0.649], h 0.050 | loss, 0.388 [0.385, 0.397], h 0.050 | reported, 0.511 [0.507, 0.518] | reported, 0.681 [0.662, 0.716] | win, 0.438 (0.271) | win, 0.451 (0.284) |
| 13 (n 128, rho 1) | loss, 0.406 [0.403, 0.408], h 0.050 | loss, 0.411 [0.410, 0.414], h 0.050 | reported, 0.411 [0.409, 0.413] | reported, 0.631 [0.616, 0.651] | loss, 1.333 (1.167) | loss, 1.336 (1.170) |
| 14 (n 128, rho 4) | loss, 0.577 [0.573, 0.580], h 0.050 | loss, 0.419 [0.416, 0.421], h 0.050 | reported, 0.506 [0.505, 0.510] | reported, 0.693 [0.652, 0.841] | win, 0.833 (0.667) | win, 0.836 (0.670) |
| 15 (n 128, rho 16) | loss, 0.646 [0.639, 0.649], h 0.050 | loss, 0.410 [0.408, 0.414], h 0.050 | reported, 0.534 [0.530, 0.538] | reported, 0.727 [0.650, 0.850] | win, 0.708 (0.542) | win, 0.712 (0.545) |
| 16 (n 128, rho 64) | loss, 0.694 [0.682, 0.696], h 0.050 | loss, 0.397 [0.389, 0.403], h 0.050 | reported, 0.545 [0.535, 0.549] | reported, 0.654 [0.624, 0.670] | win, 0.688 (0.521) | win, 0.691 (0.524) |

### Hash-M-grow against TC (against TC)

| job | creation: verdict, ratio [lo, hi], h | operation: verdict, ratio [lo, hi], h | total (reported) | read-out (reported) | B_used (end): verdict, ratio (after creation) | B_heap (end): verdict, ratio (after creation) |
| --- | --- | --- | --- | --- | --- | --- |
| 1 (n 16, rho 1) | loss, 0.170 [0.166, 0.173], h 0.050 | loss, 0.205 [0.201, 0.208], h 0.050 | reported, 0.189 [0.185, 0.192] | reported, 0.625 [0.599, 0.789] | loss, 2.083 (1.585) **severe memory loss** | loss, 2.257 (1.756) **severe memory loss** |
| 2 (n 16, rho 4) | loss, 0.381 [0.376, 0.390], h 0.050 | loss, 0.253 [0.249, 0.255], h 0.050 | reported, 0.322 [0.317, 0.326] | reported, 0.609 [0.602, 0.789] | tie, 0.958 (0.585) | loss, 1.158 (0.779) |
| 3 (n 16, rho 16) | loss, 0.514 [0.510, 0.518], h 0.053 | loss, 0.343 [0.340, 0.347], h 0.053 | reported, 0.426 [0.423, 0.433] | reported, 0.621 [0.603, 0.713] | win, 0.510 (0.335) | win, 0.705 (0.410) |
| 4 (n 16, rho 64) | loss, 0.536 [0.529, 0.541], h 0.050 | loss, 0.498 [0.494, 0.507], h 0.050 | reported, 0.521 [0.518, 0.530] | reported, 0.631 [0.611, 0.790] | win, 0.318 (0.293) | win, 0.382 (0.347) |
| 5 (n 32, rho 1) | loss, 0.297 [0.294, 0.307], h 0.050 | loss, 0.208 [0.205, 0.211], h 0.050 | reported, 0.260 [0.257, 0.266] | reported, 0.645 [0.619, 0.760] | loss, 1.333 (0.917) | loss, 1.382 (0.964) |
| 6 (n 32, rho 4) | loss, 0.353 [0.350, 0.363], h 0.050 | loss, 0.237 [0.235, 0.242], h 0.050 | reported, 0.304 [0.300, 0.307] | reported, 0.656 [0.622, 0.843] | tie, 0.958 (0.584) | tie, 1.009 (0.633) |
| 7 (n 32, rho 16) | loss, 0.533 [0.520, 0.543], h 0.050 | loss, 0.343 [0.334, 0.346], h 0.050 | reported, 0.442 [0.428, 0.448] | reported, 0.687 [0.605, 0.860] | win, 0.510 (0.334) | win, 0.563 (0.384) |
| 8 (n 32, rho 64) | loss, 0.478 [0.469, 0.482], h 0.050 | loss, 0.303 [0.298, 0.307], h 0.050 | reported, 0.399 [0.394, 0.405] | reported, 0.704 [0.639, 0.850] | win, 0.357 (0.271) | win, 0.407 (0.291) |
| 9 (n 64, rho 1) | loss, 0.373 [0.370, 0.374], h 0.050 | loss, 0.325 [0.322, 0.326], h 0.050 | reported, 0.356 [0.354, 0.358] | reported, 0.792 [0.660, 0.893] | loss, 1.167 (0.917) | loss, 1.179 (0.929) |
| 10 (n 64, rho 4) | loss, 0.533 [0.530, 0.535], h 0.050 | loss, 0.378 [0.377, 0.381], h 0.050 | reported, 0.465 [0.463, 0.466] | reported, 0.660 [0.651, 0.676] | win, 0.604 (0.417) | win, 0.617 (0.429) |
| 11 (n 64, rho 16) | loss, 0.599 [0.591, 0.604], h 0.050 | loss, 0.390 [0.385, 0.395], h 0.050 | reported, 0.500 [0.493, 0.505] | reported, 0.684 [0.646, 0.782] | win, 0.510 (0.333) | win, 0.524 (0.346) |
| 12 (n 64, rho 64) | loss, 0.658 [0.652, 0.666], h 0.050 | loss, 0.395 [0.392, 0.406], h 0.050 | reported, 0.521 [0.516, 0.531] | reported, 0.685 [0.663, 0.720] | win, 0.440 (0.271) | win, 0.454 (0.284) |
| 13 (n 128, rho 1) | loss, 0.406 [0.405, 0.408], h 0.050 | loss, 0.356 [0.353, 0.358], h 0.050 | reported, 0.389 [0.388, 0.391] | reported, 0.585 [0.567, 0.612] | loss, 1.500 (1.167) | loss, 1.503 (1.170) |
| 14 (n 128, rho 4) | loss, 0.578 [0.549, 0.582], h 0.050 | loss, 0.396 [0.374, 0.399], h 0.050 | reported, 0.494 [0.468, 0.498] | reported, 0.689 [0.612, 0.837] | win, 0.875 (0.667) | win, 0.878 (0.670) |
| 15 (n 128, rho 16) | loss, 0.653 [0.647, 0.658], h 0.050 | loss, 0.409 [0.405, 0.410], h 0.050 | reported, 0.535 [0.532, 0.539] | reported, 0.729 [0.655, 0.855] | win, 0.719 (0.542) | win, 0.722 (0.545) |
| 16 (n 128, rho 64) | loss, 0.696 [0.691, 0.701], h 0.050 | loss, 0.399 [0.396, 0.405], h 0.050 | reported, 0.547 [0.544, 0.552] | reported, 0.672 [0.640, 0.683] | win, 0.693 (0.521) | win, 0.696 (0.524) |

### Hash-M-fixed against Hash-V-fixed (M against V)

| job | creation: verdict, ratio [lo, hi], h | operation: verdict, ratio [lo, hi], h | total (reported) | read-out (reported) | B_used (end): verdict, ratio (after creation) | B_heap (end): verdict, ratio (after creation) |
| --- | --- | --- | --- | --- | --- | --- |
| 1 (n 16, rho 1) | tie, 0.972 [0.951, 0.992], h 0.050 | loss, 0.733 [0.726, 0.744], h 0.050 | reported, 0.845 [0.830, 0.855] | reported, 0.983 [0.961, 1.006] | tie, 1.087 (1.000) | tie, 1.084 (1.002) |
| 2 (n 16, rho 4) | tie, 0.997 [0.986, 1.003], h 0.050 | loss, 0.809 [0.804, 0.820], h 0.050 | reported, 0.897 [0.888, 0.904] | reported, 0.991 [0.977, 0.998] | tie, 1.087 (1.000) | tie, 1.084 (1.002) |
| 3 (n 16, rho 16) | tie, 1.007 [0.996, 1.029], h 0.053 | loss, 0.868 [0.864, 0.878], h 0.053 | reported, 0.934 [0.926, 0.949] | reported, 0.986 [0.978, 1.004] | tie, 1.087 (1.000) | tie, 1.084 (1.002) |
| 4 (n 16, rho 64) | tie, 1.009 [1.000, 1.019], h 0.050 | loss, 0.894 [0.887, 0.899], h 0.050 | reported, 0.950 [0.943, 0.955] | reported, 0.992 [0.982, 0.998] | tie, 1.087 (1.000) | tie, 1.084 (1.002) |
| 5 (n 32, rho 1) | tie, 1.002 [0.992, 1.013], h 0.050 | loss, 0.753 [0.749, 0.764], h 0.050 | reported, 0.880 [0.872, 0.888] | reported, 0.997 [0.980, 1.009] | tie, 1.087 (1.000) | tie, 1.087 (1.001) |
| 6 (n 32, rho 4) | tie, 1.036 [1.030, 1.050], h 0.050 | loss, 0.853 [0.850, 0.857], h 0.050 | reported, 0.951 [0.945, 0.959] | reported, 0.989 [0.976, 1.002] | tie, 1.087 (1.000) | tie, 1.087 (1.001) |
| 7 (n 32, rho 16) | win, 1.102 [1.075, 1.125], h 0.050 | loss, 0.897 [0.885, 0.921], h 0.050 | reported, 1.002 [0.984, 1.024] | reported, 0.983 [0.942, 1.000] | tie, 1.087 (1.000) | tie, 1.087 (1.001) |
| 8 (n 32, rho 64) | win, 1.123 [1.112, 1.133], h 0.050 | loss, 0.932 [0.930, 0.938], h 0.050 | reported, 1.028 [1.024, 1.035] | reported, 0.991 [0.985, 1.005] | tie, 1.087 (1.000) | tie, 1.087 (1.001) |
| 9 (n 64, rho 1) | tie, 1.043 [1.039, 1.047], h 0.050 | loss, 0.756 [0.753, 0.760], h 0.050 | reported, 0.905 [0.902, 0.909] | reported, 0.960 [0.941, 0.998] | tie, 1.087 (1.000) | tie, 1.087 (1.000) |
| 10 (n 64, rho 4) | win, 1.061 [1.058, 1.065], h 0.050 | loss, 0.868 [0.866, 0.872], h 0.050 | reported, 0.974 [0.972, 0.976] | reported, 0.979 [0.961, 1.001] | tie, 1.087 (1.000) | tie, 1.087 (1.000) |
| 11 (n 64, rho 16) | win, 1.058 [1.051, 1.069], h 0.050 | loss, 0.922 [0.917, 0.928], h 0.050 | reported, 0.994 [0.989, 1.004] | reported, 0.958 [0.939, 0.996] | tie, 1.087 (1.000) | tie, 1.087 (1.000) |
| 12 (n 64, rho 64) | tie, 1.049 [1.043, 1.057], h 0.050 | loss, 0.928 [0.919, 0.940], h 0.050 | reported, 0.990 [0.985, 0.999] | reported, 0.966 [0.950, 0.980] | tie, 1.087 (1.000) | tie, 1.087 (1.000) |
| 13 (n 128, rho 1) | tie, 1.046 [1.043, 1.050], h 0.050 | loss, 0.792 [0.780, 0.803], h 0.050 | reported, 0.927 [0.916, 0.933] | reported, 0.963 [0.950, 0.987] | loss, 1.154 (1.000) | loss, 1.154 (1.000) |
| 14 (n 128, rho 4) | win, 1.058 [1.052, 1.068], h 0.050 | loss, 0.863 [0.849, 0.870], h 0.050 | reported, 0.970 [0.959, 0.977] | reported, 0.994 [0.965, 1.019] | loss, 1.154 (1.000) | loss, 1.154 (1.000) |
| 15 (n 128, rho 16) | win, 1.060 [1.050, 1.075], h 0.050 | loss, 0.898 [0.891, 0.909], h 0.050 | reported, 0.982 [0.979, 0.998] | reported, 0.992 [0.949, 1.024] | loss, 1.154 (1.000) | loss, 1.154 (1.000) |
| 16 (n 128, rho 64) | win, 1.060 [1.056, 1.068], h 0.050 | loss, 0.911 [0.901, 0.916], h 0.050 | reported, 0.991 [0.988, 0.995] | reported, 0.960 [0.933, 0.979] | loss, 1.154 (1.000) | loss, 1.154 (1.000) |

### Hash-M-grow against Hash-V-grow (M against V)

| job | creation: verdict, ratio [lo, hi], h | operation: verdict, ratio [lo, hi], h | total (reported) | read-out (reported) | B_used (end): verdict, ratio (after creation) | B_heap (end): verdict, ratio (after creation) |
| --- | --- | --- | --- | --- | --- | --- |
| 1 (n 16, rho 1) | tie, 1.029 [1.005, 1.047], h 0.050 | loss, 0.792 [0.785, 0.803], h 0.050 | reported, 0.938 [0.921, 0.949] | reported, 0.995 [0.983, 1.004] | tie, 1.087 (1.000) | tie, 1.080 (1.002) |
| 2 (n 16, rho 4) | tie, 1.000 [0.987, 1.022], h 0.050 | tie, 1.001 [0.986, 1.018], h 0.050 | reported, 1.004 [0.988, 1.019] | reported, 0.990 [0.985, 1.012] | tie, 1.045 (1.000) | tie, 1.041 (1.005) |
| 3 (n 16, rho 16) | tie, 1.032 [1.015, 1.037], h 0.053 | tie, 1.008 [0.998, 1.018], h 0.053 | reported, 1.018 [1.007, 1.026] | reported, 1.007 [0.973, 1.023] | tie, 1.021 (1.000) | tie, 1.005 (1.009) |
| 4 (n 16, rho 64) | tie, 1.018 [1.010, 1.039], h 0.050 | tie, 0.985 [0.978, 0.997], h 0.050 | reported, 1.003 [0.996, 1.018] | reported, 1.003 [0.992, 1.032] | tie, 1.017 (1.000) | tie, 1.027 (1.011) |
| 5 (n 32, rho 1) | tie, 0.993 [0.989, 1.004], h 0.050 | loss, 0.854 [0.849, 0.862], h 0.050 | reported, 0.930 [0.924, 0.937] | reported, 1.006 [0.987, 1.017] | tie, 1.067 (1.000) | tie, 1.065 (1.001) |
| 6 (n 32, rho 4) | tie, 0.997 [0.994, 1.002], h 0.050 | tie, 0.951 [0.945, 0.959], h 0.050 | reported, 0.977 [0.972, 0.981] | reported, 0.997 [0.985, 1.016] | tie, 1.045 (1.000) | tie, 1.044 (1.002) |
| 7 (n 32, rho 16) | tie, 1.054 [1.031, 1.067], h 0.050 | tie, 0.956 [0.936, 0.962], h 0.050 | reported, 1.005 [0.983, 1.013] | reported, 0.999 [0.934, 1.019] | tie, 1.021 (1.000) | tie, 1.021 (1.003) |
| 8 (n 32, rho 64) | win, 1.064 [1.057, 1.072], h 0.050 | tie, 0.990 [0.976, 0.998], h 0.050 | reported, 1.030 [1.018, 1.033] | reported, 0.999 [0.986, 1.015] | tie, 1.007 (1.000) | tie, 1.002 (1.003) |
| 9 (n 64, rho 1) | tie, 0.998 [0.994, 1.001], h 0.050 | loss, 0.828 [0.821, 0.832], h 0.050 | reported, 0.929 [0.924, 0.931] | reported, 0.999 [0.967, 1.016] | tie, 1.077 (1.000) | tie, 1.076 (1.000) |
| 10 (n 64, rho 4) | tie, 1.008 [1.006, 1.012], h 0.050 | tie, 0.960 [0.958, 0.963], h 0.050 | reported, 0.986 [0.984, 0.988] | reported, 0.997 [0.985, 1.006] | tie, 1.036 (1.000) | tie, 1.035 (1.001) |
| 11 (n 64, rho 16) | tie, 1.010 [0.996, 1.020], h 0.050 | tie, 1.002 [0.989, 1.007], h 0.050 | reported, 1.006 [0.991, 1.012] | reported, 0.980 [0.965, 1.005] | tie, 1.021 (1.000) | tie, 1.021 (1.001) |
| 12 (n 64, rho 64) | tie, 1.023 [1.013, 1.031], h 0.050 | tie, 1.018 [1.013, 1.026], h 0.050 | reported, 1.020 [1.013, 1.029] | reported, 1.006 [0.979, 1.021] | tie, 1.006 (1.000) | tie, 1.006 (1.001) |
| 13 (n 128, rho 1) | tie, 0.999 [0.997, 1.007], h 0.050 | loss, 0.865 [0.860, 0.870], h 0.050 | reported, 0.946 [0.943, 0.951] | reported, 0.928 [0.901, 0.980] | loss, 1.125 (1.000) | loss, 1.125 (1.000) |
| 14 (n 128, rho 4) | tie, 1.003 [0.954, 1.011], h 0.050 | tie, 0.945 [0.892, 0.954], h 0.050 | reported, 0.977 [0.925, 0.983] | reported, 0.993 [0.887, 1.009] | tie, 1.050 (1.000) | tie, 1.050 (1.000) |
| 15 (n 128, rho 16) | tie, 1.011 [1.004, 1.023], h 0.050 | tie, 0.996 [0.985, 0.999], h 0.050 | reported, 1.002 [0.995, 1.011] | reported, 1.002 [0.970, 1.030] | tie, 1.015 (1.000) | tie, 1.015 (1.000) |
| 16 (n 128, rho 64) | tie, 1.003 [0.995, 1.023], h 0.050 | tie, 1.004 [0.998, 1.023], h 0.050 | reported, 1.004 [0.998, 1.023] | reported, 1.026 [0.988, 1.060] | tie, 1.008 (1.000) | tie, 1.008 (1.000) |

### Hash-V-grow against Hash-V-fixed (grow against fixed)

| job | creation: verdict, ratio [lo, hi], h | operation: verdict, ratio [lo, hi], h | total (reported) | read-out (reported) | B_used (end): verdict, ratio (after creation) | B_heap (end): verdict, ratio (after creation) |
| --- | --- | --- | --- | --- | --- | --- |
| 1 (n 16, rho 1) | loss, 0.293 [0.288, 0.297], h 0.050 | loss, 0.573 [0.568, 0.579], h 0.050 | reported, 0.393 [0.389, 0.399] | reported, 0.995 [0.987, 1.011] | tie, 1.000 (0.951) | tie, 1.050 (1.054) |
| 2 (n 16, rho 4) | loss, 0.633 [0.620, 0.637], h 0.050 | loss, 0.592 [0.585, 0.601], h 0.050 | reported, 0.619 [0.611, 0.626] | reported, 0.998 [0.978, 1.006] | win, 0.478 (0.351) | win, 0.559 (0.466) |
| 3 (n 16, rho 16) | loss, 0.814 [0.806, 0.833], h 0.053 | loss, 0.715 [0.710, 0.724], h 0.053 | reported, 0.767 [0.760, 0.781] | reported, 0.988 [0.971, 1.010] | win, 0.261 (0.201) | win, 0.352 (0.244) |
| 4 (n 16, rho 64) | loss, 0.858 [0.839, 0.868], h 0.050 | tie, 0.945 [0.936, 0.953], h 0.050 | reported, 0.902 [0.889, 0.909] | reported, 0.999 [0.972, 1.005] | win, 0.163 (0.176) | win, 0.187 (0.207) |
| 5 (n 32, rho 1) | loss, 0.503 [0.498, 0.509], h 0.050 | loss, 0.519 [0.515, 0.524], h 0.050 | reported, 0.516 [0.511, 0.519] | reported, 1.014 [0.995, 1.024] | win, 0.652 (0.550) | win, 0.678 (0.578) |
| 6 (n 32, rho 4) | loss, 0.610 [0.606, 0.617], h 0.050 | loss, 0.515 [0.512, 0.520], h 0.050 | reported, 0.570 [0.568, 0.574] | reported, 1.000 [0.979, 1.009] | win, 0.478 (0.350) | win, 0.505 (0.379) |
| 7 (n 32, rho 16) | loss, 0.833 [0.826, 0.852], h 0.050 | loss, 0.699 [0.690, 0.716], h 0.050 | reported, 0.770 [0.765, 0.789] | reported, 0.998 [0.985, 1.015] | win, 0.261 (0.200) | win, 0.288 (0.230) |
| 8 (n 32, rho 64) | loss, 0.870 [0.862, 0.876], h 0.050 | loss, 0.786 [0.779, 0.791], h 0.050 | reported, 0.833 [0.827, 0.838] | reported, 1.006 [0.989, 1.018] | win, 0.185 (0.163) | win, 0.212 (0.174) |
| 9 (n 64, rho 1) | loss, 0.581 [0.580, 0.583], h 0.050 | loss, 0.675 [0.671, 0.678], h 0.050 | reported, 0.619 [0.618, 0.622] | reported, 0.999 [0.987, 1.025] | win, 0.565 (0.550) | win, 0.572 (0.557) |
| 10 (n 64, rho 4) | loss, 0.810 [0.809, 0.813], h 0.050 | loss, 0.676 [0.674, 0.678], h 0.050 | reported, 0.754 [0.752, 0.755] | reported, 1.010 [0.997, 1.020] | win, 0.304 (0.250) | win, 0.311 (0.258) |
| 11 (n 64, rho 16) | loss, 0.876 [0.866, 0.883], h 0.050 | loss, 0.678 [0.673, 0.687], h 0.050 | reported, 0.781 [0.777, 0.790] | reported, 0.999 [0.991, 1.016] | win, 0.261 (0.200) | win, 0.268 (0.208) |
| 12 (n 64, rho 64) | loss, 0.940 [0.937, 0.946], h 0.050 | loss, 0.701 [0.694, 0.709], h 0.050 | reported, 0.816 [0.812, 0.822] | reported, 0.991 [0.972, 1.013] | win, 0.228 (0.163) | win, 0.235 (0.170) |
| 13 (n 128, rho 1) | loss, 0.632 [0.628, 0.634], h 0.050 | loss, 0.723 [0.721, 0.732], h 0.050 | reported, 0.670 [0.666, 0.672] | reported, 1.009 [0.981, 1.029] | win, 0.615 (0.609) | win, 0.617 (0.610) |
| 14 (n 128, rho 4) | loss, 0.855 [0.850, 0.866], h 0.050 | loss, 0.710 [0.699, 0.718], h 0.050 | reported, 0.792 [0.784, 0.801] | reported, 1.021 [1.005, 1.047] | win, 0.385 (0.348) | win, 0.386 (0.349) |
| 15 (n 128, rho 16) | loss, 0.915 [0.905, 0.926], h 0.050 | loss, 0.712 [0.707, 0.722], h 0.050 | reported, 0.818 [0.813, 0.830] | reported, 1.029 [0.997, 1.058] | win, 0.327 (0.283) | win, 0.328 (0.284) |
| 16 (n 128, rho 64) | tie, 0.981 [0.964, 0.988], h 0.050 | loss, 0.693 [0.679, 0.697], h 0.050 | reported, 0.837 [0.821, 0.842] | reported, 0.987 [0.956, 1.011] | win, 0.317 (0.272) | win, 0.319 (0.273) |

### Hash-M-grow against Hash-M-fixed (grow against fixed)

| job | creation: verdict, ratio [lo, hi], h | operation: verdict, ratio [lo, hi], h | total (reported) | read-out (reported) | B_used (end): verdict, ratio (after creation) | B_heap (end): verdict, ratio (after creation) |
| --- | --- | --- | --- | --- | --- | --- |
| 1 (n 16, rho 1) | loss, 0.310 [0.304, 0.318], h 0.050 | loss, 0.620 [0.615, 0.629], h 0.050 | reported, 0.436 [0.431, 0.444] | reported, 1.007 [0.991, 1.034] | tie, 1.000 (0.951) | tie, 1.046 (1.053) |
| 2 (n 16, rho 4) | loss, 0.635 [0.628, 0.645], h 0.050 | loss, 0.732 [0.719, 0.742], h 0.050 | reported, 0.693 [0.683, 0.700] | reported, 0.998 [0.993, 1.019] | win, 0.460 (0.351) | win, 0.537 (0.467) |
| 3 (n 16, rho 16) | loss, 0.834 [0.824, 0.842], h 0.053 | loss, 0.830 [0.821, 0.839], h 0.053 | reported, 0.836 [0.827, 0.843] | reported, 1.010 [0.977, 1.017] | win, 0.245 (0.201) | win, 0.327 (0.246) |
| 4 (n 16, rho 64) | loss, 0.866 [0.861, 0.870], h 0.050 | tie, 1.042 [1.034, 1.053], h 0.050 | reported, 0.952 [0.949, 0.958] | reported, 1.010 [1.000, 1.020] | win, 0.152 (0.176) | win, 0.177 (0.208) |
| 5 (n 32, rho 1) | loss, 0.499 [0.496, 0.504], h 0.050 | loss, 0.589 [0.583, 0.593], h 0.050 | reported, 0.545 [0.540, 0.549] | reported, 1.022 [1.004, 1.034] | win, 0.640 (0.550) | win, 0.664 (0.579) |
| 6 (n 32, rho 4) | loss, 0.587 [0.581, 0.590], h 0.050 | loss, 0.575 [0.571, 0.580], h 0.050 | reported, 0.586 [0.581, 0.589] | reported, 1.009 [0.994, 1.016] | win, 0.460 (0.350) | win, 0.485 (0.380) |
| 7 (n 32, rho 16) | loss, 0.797 [0.779, 0.817], h 0.050 | loss, 0.745 [0.725, 0.753], h 0.050 | reported, 0.772 [0.756, 0.787] | reported, 1.014 [0.964, 1.047] | win, 0.245 (0.200) | win, 0.270 (0.231) |
| 8 (n 32, rho 64) | loss, 0.825 [0.819, 0.828], h 0.050 | loss, 0.835 [0.825, 0.838], h 0.050 | reported, 0.835 [0.827, 0.837] | reported, 1.015 [0.990, 1.023] | win, 0.171 (0.163) | win, 0.196 (0.174) |
| 9 (n 64, rho 1) | loss, 0.556 [0.554, 0.558], h 0.050 | loss, 0.739 [0.732, 0.741], h 0.050 | reported, 0.635 [0.632, 0.637] | reported, 1.039 [0.993, 1.058] | win, 0.560 (0.550) | win, 0.566 (0.557) |
| 10 (n 64, rho 4) | loss, 0.770 [0.767, 0.773], h 0.050 | loss, 0.748 [0.746, 0.750], h 0.050 | reported, 0.763 [0.761, 0.765] | reported, 1.029 [1.008, 1.045] | win, 0.290 (0.250) | win, 0.296 (0.258) |
| 11 (n 64, rho 16) | loss, 0.836 [0.826, 0.840], h 0.050 | loss, 0.737 [0.726, 0.743], h 0.050 | reported, 0.790 [0.780, 0.796] | reported, 1.023 [0.992, 1.054] | win, 0.245 (0.200) | win, 0.251 (0.208) |
| 12 (n 64, rho 64) | loss, 0.917 [0.909, 0.927], h 0.050 | loss, 0.769 [0.765, 0.775], h 0.050 | reported, 0.841 [0.835, 0.848] | reported, 1.032 [1.012, 1.047] | win, 0.211 (0.163) | win, 0.218 (0.170) |
| 13 (n 128, rho 1) | loss, 0.603 [0.602, 0.607], h 0.050 | loss, 0.791 [0.782, 0.803], h 0.050 | reported, 0.684 [0.679, 0.692] | reported, 0.972 [0.938, 1.011] | win, 0.600 (0.609) | win, 0.601 (0.610) |
| 14 (n 128, rho 4) | loss, 0.811 [0.771, 0.816], h 0.050 | loss, 0.777 [0.732, 0.785], h 0.050 | reported, 0.797 [0.754, 0.803] | reported, 1.021 [0.931, 1.034] | win, 0.350 (0.348) | win, 0.351 (0.349) |
| 15 (n 128, rho 16) | loss, 0.873 [0.866, 0.879], h 0.050 | loss, 0.790 [0.782, 0.793], h 0.050 | reported, 0.835 [0.829, 0.840] | reported, 1.040 [1.008, 1.062] | win, 0.287 (0.283) | win, 0.289 (0.284) |
| 16 (n 128, rho 64) | loss, 0.928 [0.922, 0.932], h 0.050 | loss, 0.763 [0.760, 0.771], h 0.050 | reported, 0.848 [0.844, 0.852] | reported, 1.055 [1.010, 1.092] | win, 0.277 (0.272) | win, 0.278 (0.273) |

### Hash-V-grow-map against Hash-V-grow (map against lookup)

| job | creation: verdict, ratio [lo, hi], h | operation: verdict, ratio [lo, hi], h | total (reported) | read-out (reported) | B_used (end): verdict, ratio (after creation) | B_heap (end): verdict, ratio (after creation) |
| --- | --- | --- | --- | --- | --- | --- |
| 1 (n 16, rho 1) | win, 1.485 [1.453, 1.500], h 0.050 | reported, 1.162 [1.137, 1.176] | reported, 1.354 [1.328, 1.367] | reported, 0.980 [0.964, 0.999] | reported, 1.000 (1.000) | reported, 1.000 (1.006) |
| 2 (n 16, rho 4) | win, 1.177 [1.159, 1.203], h 0.050 | reported, 1.230 [1.205, 1.245] | reported, 1.202 [1.175, 1.219] | reported, 0.986 [0.980, 1.007] | reported, 1.000 (1.000) | reported, 1.000 (1.013) |
| 3 (n 16, rho 16) | tie, 1.053 [1.024, 1.062], h 0.053 | reported, 1.140 [1.087, 1.154] | reported, 1.093 [1.042, 1.108] | reported, 0.991 [0.978, 1.007] | reported, 1.000 (1.000) | reported, 1.005 (1.025) |
| 4 (n 16, rho 64) | tie, 1.021 [1.015, 1.040], h 0.050 | reported, 0.987 [0.978, 0.999] | reported, 1.007 [1.000, 1.020] | reported, 1.001 [0.991, 1.021] | reported, 1.000 (1.000) | reported, 1.010 (1.011) |
| 5 (n 32, rho 1) | win, 1.212 [1.197, 1.221], h 0.050 | reported, 1.193 [1.186, 1.202] | reported, 1.200 [1.191, 1.209] | reported, 0.967 [0.944, 0.998] | reported, 1.000 (1.000) | reported, 1.000 (1.003) |
| 6 (n 32, rho 4) | win, 1.216 [1.211, 1.224], h 0.050 | reported, 1.187 [1.177, 1.196] | reported, 1.202 [1.193, 1.206] | reported, 0.983 [0.976, 0.998] | reported, 1.000 (1.000) | reported, 1.000 (1.004) |
| 7 (n 32, rho 16) | tie, 1.066 [1.049, 1.077], h 0.050 | reported, 1.042 [1.026, 1.053] | reported, 1.057 [1.038, 1.062] | reported, 0.975 [0.952, 0.987] | reported, 1.000 (1.000) | reported, 1.002 (1.007) |
| 8 (n 32, rho 64) | win, 1.065 [1.058, 1.078], h 0.050 | reported, 1.040 [1.024, 1.048] | reported, 1.052 [1.044, 1.058] | reported, 0.934 [0.927, 0.959] | reported, 1.000 (1.000) | reported, 1.002 (1.009) |
| 9 (n 64, rho 1) | win, 1.187 [1.182, 1.189], h 0.050 | reported, 1.098 [1.094, 1.104] | reported, 1.151 [1.147, 1.154] | reported, 0.946 [0.913, 0.970] | reported, 1.000 (1.000) | reported, 1.000 (1.001) |
| 10 (n 64, rho 4) | win, 1.072 [1.069, 1.075], h 0.050 | reported, 1.104 [1.102, 1.106] | reported, 1.084 [1.082, 1.086] | reported, 0.947 [0.926, 0.954] | reported, 1.000 (1.000) | reported, 1.000 (1.002) |
| 11 (n 64, rho 16) | win, 1.060 [1.052, 1.070], h 0.050 | reported, 1.103 [1.091, 1.112] | reported, 1.082 [1.071, 1.086] | reported, 0.979 [0.947, 0.997] | reported, 1.000 (1.000) | reported, 1.000 (1.002) |
| 12 (n 64, rho 64) | tie, 1.017 [1.008, 1.022], h 0.050 | reported, 1.121 [1.112, 1.130] | reported, 1.067 [1.060, 1.075] | reported, 0.976 [0.957, 0.994] | reported, 1.000 (1.000) | reported, 1.000 (1.002) |
| 13 (n 128, rho 1) | win, 1.157 [1.146, 1.167], h 0.050 | reported, 1.092 [1.086, 1.100] | reported, 1.131 [1.121, 1.139] | reported, 1.025 [0.988, 1.061] | reported, 1.000 (1.000) | reported, 1.002 (1.000) |
| 14 (n 128, rho 4) | tie, 1.051 [1.037, 1.057], h 0.050 | reported, 1.093 [1.078, 1.097] | reported, 1.069 [1.055, 1.073] | reported, 0.996 [0.969, 1.006] | reported, 1.000 (1.000) | reported, 1.003 (1.000) |
| 15 (n 128, rho 16) | tie, 1.013 [1.008, 1.023], h 0.050 | reported, 1.101 [1.091, 1.105] | reported, 1.051 [1.047, 1.057] | reported, 1.000 [0.987, 1.017] | reported, 1.000 (1.000) | reported, 1.003 (1.000) |
| 16 (n 128, rho 64) | tie, 1.002 [0.989, 1.021], h 0.050 | reported, 1.102 [1.096, 1.124] | reported, 1.050 [1.041, 1.069] | reported, 1.008 [0.961, 1.054] | reported, 1.000 (1.000) | reported, 1.004 (1.000) |

### Gcd work (a check, not a verdict) and the call-for-call checks

G(row)/G(TC) MUST be exactly 1 for every hash row (rule-m1 item 7); the counting runs' per-operation records of every hash row equal TC's, and V against M, grow against fixed and map against lookup take the same cases, core counts and store splits (spec section 10 items 3-6).

| job | G per operation | G(Hash-V-fixed)/G(TC), call for call | G(Hash-M-fixed)/G(TC), call for call | G(Hash-V-grow)/G(TC), call for call | G(Hash-M-grow)/G(TC), call for call | G(Hash-V-grow-map)/G(TC), call for call | M vs V (fixed, grow) | grow vs fixed (V, M) | map vs lookup |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | 42.89 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass, pass | pass, pass | pass |
| 2 | 41.56 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass, pass | pass, pass | pass |
| 3 | 43.43 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass, pass | pass, pass | pass |
| 4 | 47.53 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass, pass | pass, pass | pass |
| 5 | 42.24 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass, pass | pass, pass | pass |
| 6 | 42.47 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass, pass | pass, pass | pass |
| 7 | 43.54 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass, pass | pass, pass | pass |
| 8 | 38.81 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass, pass | pass, pass | pass |
| 9 | 42.22 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass, pass | pass, pass | pass |
| 10 | 41.94 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass, pass | pass, pass | pass |
| 11 | 41.43 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass, pass | pass, pass | pass |
| 12 | 42.82 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass, pass | pass, pass | pass |
| 13 | 42.13 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass, pass | pass, pass | pass |
| 14 | 42.16 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass, pass | pass, pass | pass |
| 15 | 41.94 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass, pass | pass, pass | pass |
| 16 | 41.14 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass, pass | pass, pass | pass |

### Strata (counts of verdicts; no measure combined, no stratum scored)

The strata are the sharing levels of A and B (rule-m1 item 11): NONE (rho = 1), LOW (rho = 4), MID (rho = 16), HIGH (rho = 64).

| stratum | pairing | measure | wins | ties | losses | over cap | n/a | reported | median ratio | severe flags |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| NONE | Hash-V-fixed vs TC | creation | 0 | 0 | 4 | 0 | 0 | 0 | 0.618 | 0 |
| NONE | Hash-V-fixed vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.519 | 0 |
| NONE | Hash-V-fixed vs TC | B_used | 0 | 0 | 4 | 0 | 0 | 0 | 1.917 | 1 |
| NONE | Hash-V-fixed vs TC | B_heap | 0 | 0 | 4 | 0 | 0 | 0 | 1.953 | 1 |
| NONE | Hash-M-fixed vs TC | creation | 0 | 0 | 4 | 0 | 0 | 0 | 0.633 | 0 |
| NONE | Hash-M-fixed vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.397 | 0 |
| NONE | Hash-M-fixed vs TC | B_used | 0 | 0 | 4 | 0 | 0 | 0 | 2.083 | 4 |
| NONE | Hash-M-fixed vs TC | B_heap | 0 | 0 | 4 | 0 | 0 | 0 | 2.120 | 4 |
| NONE | Hash-V-grow vs TC | creation | 0 | 0 | 4 | 0 | 0 | 0 | 0.336 | 0 |
| NONE | Hash-V-grow vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.325 | 0 |
| NONE | Hash-V-grow vs TC | B_used | 0 | 1 | 3 | 0 | 0 | 0 | 1.292 | 0 |
| NONE | Hash-V-grow vs TC | B_heap | 0 | 1 | 3 | 0 | 0 | 0 | 1.317 | 1 |
| NONE | Hash-M-grow vs TC | creation | 0 | 0 | 4 | 0 | 0 | 0 | 0.335 | 0 |
| NONE | Hash-M-grow vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.266 | 0 |
| NONE | Hash-M-grow vs TC | B_used | 0 | 0 | 4 | 0 | 0 | 0 | 1.417 | 1 |
| NONE | Hash-M-grow vs TC | B_heap | 0 | 0 | 4 | 0 | 0 | 0 | 1.442 | 1 |
| NONE | Hash-M-fixed vs Hash-V-fixed | creation | 0 | 4 | 0 | 0 | 0 | 0 | 1.023 | 0 |
| NONE | Hash-M-fixed vs Hash-V-fixed | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.755 | 0 |
| NONE | Hash-M-fixed vs Hash-V-fixed | B_used | 0 | 3 | 1 | 0 | 0 | 0 | 1.087 | 0 |
| NONE | Hash-M-fixed vs Hash-V-fixed | B_heap | 0 | 3 | 1 | 0 | 0 | 0 | 1.087 | 0 |
| NONE | Hash-M-grow vs Hash-V-grow | creation | 0 | 4 | 0 | 0 | 0 | 0 | 0.998 | 0 |
| NONE | Hash-M-grow vs Hash-V-grow | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.841 | 0 |
| NONE | Hash-M-grow vs Hash-V-grow | B_used | 0 | 3 | 1 | 0 | 0 | 0 | 1.082 | 0 |
| NONE | Hash-M-grow vs Hash-V-grow | B_heap | 0 | 3 | 1 | 0 | 0 | 0 | 1.078 | 0 |
| NONE | Hash-V-grow vs Hash-V-fixed | creation | 0 | 0 | 4 | 0 | 0 | 0 | 0.542 | 0 |
| NONE | Hash-V-grow vs Hash-V-fixed | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.624 | 0 |
| NONE | Hash-V-grow vs Hash-V-fixed | B_used | 3 | 1 | 0 | 0 | 0 | 0 | 0.634 | 0 |
| NONE | Hash-V-grow vs Hash-V-fixed | B_heap | 3 | 1 | 0 | 0 | 0 | 0 | 0.647 | 0 |
| NONE | Hash-M-grow vs Hash-M-fixed | creation | 0 | 0 | 4 | 0 | 0 | 0 | 0.527 | 0 |
| NONE | Hash-M-grow vs Hash-M-fixed | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.679 | 0 |
| NONE | Hash-M-grow vs Hash-M-fixed | B_used | 3 | 1 | 0 | 0 | 0 | 0 | 0.620 | 0 |
| NONE | Hash-M-grow vs Hash-M-fixed | B_heap | 3 | 1 | 0 | 0 | 0 | 0 | 0.633 | 0 |
| NONE | Hash-V-grow-map vs Hash-V-grow | creation | 4 | 0 | 0 | 0 | 0 | 0 | 1.200 | 0 |
| NONE | Hash-V-grow-map vs Hash-V-grow | operation | 0 | 0 | 0 | 0 | 0 | 4 | 1.130 | 0 |
| NONE | Hash-V-grow-map vs Hash-V-grow | B_used | 0 | 0 | 0 | 0 | 0 | 4 | 1.000 | 0 |
| NONE | Hash-V-grow-map vs Hash-V-grow | B_heap | 0 | 0 | 0 | 0 | 0 | 4 | 1.000 | 0 |
| LOW | Hash-V-fixed vs TC | creation | 0 | 0 | 4 | 0 | 0 | 0 | 0.627 | 0 |
| LOW | Hash-V-fixed vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.534 | 0 |
| LOW | Hash-V-fixed vs TC | B_used | 0 | 0 | 4 | 0 | 0 | 0 | 1.917 | 1 |
| LOW | Hash-V-fixed vs TC | B_heap | 0 | 0 | 4 | 0 | 0 | 0 | 1.953 | 1 |
| LOW | Hash-M-fixed vs TC | creation | 0 | 0 | 4 | 0 | 0 | 0 | 0.647 | 0 |
| LOW | Hash-M-fixed vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.460 | 0 |
| LOW | Hash-M-fixed vs TC | B_used | 0 | 0 | 4 | 0 | 0 | 0 | 2.083 | 4 |
| LOW | Hash-M-fixed vs TC | B_heap | 0 | 0 | 4 | 0 | 0 | 0 | 2.120 | 4 |
| LOW | Hash-V-grow vs TC | creation | 0 | 0 | 4 | 0 | 0 | 0 | 0.455 | 0 |
| LOW | Hash-V-grow vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.323 | 0 |
| LOW | Hash-V-grow vs TC | B_used | 2 | 2 | 0 | 0 | 0 | 0 | 0.875 | 0 |
| LOW | Hash-V-grow vs TC | B_heap | 2 | 1 | 1 | 0 | 0 | 0 | 0.901 | 0 |
| LOW | Hash-M-grow vs TC | creation | 0 | 0 | 4 | 0 | 0 | 0 | 0.457 | 0 |
| LOW | Hash-M-grow vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.315 | 0 |
| LOW | Hash-M-grow vs TC | B_used | 2 | 2 | 0 | 0 | 0 | 0 | 0.917 | 0 |
| LOW | Hash-M-grow vs TC | B_heap | 2 | 1 | 1 | 0 | 0 | 0 | 0.944 | 0 |
| LOW | Hash-M-fixed vs Hash-V-fixed | creation | 2 | 2 | 0 | 0 | 0 | 0 | 1.047 | 0 |
| LOW | Hash-M-fixed vs Hash-V-fixed | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.858 | 0 |
| LOW | Hash-M-fixed vs Hash-V-fixed | B_used | 0 | 3 | 1 | 0 | 0 | 0 | 1.087 | 0 |
| LOW | Hash-M-fixed vs Hash-V-fixed | B_heap | 0 | 3 | 1 | 0 | 0 | 0 | 1.087 | 0 |
| LOW | Hash-M-grow vs Hash-V-grow | creation | 0 | 4 | 0 | 0 | 0 | 0 | 1.001 | 0 |
| LOW | Hash-M-grow vs Hash-V-grow | operation | 0 | 4 | 0 | 0 | 0 | 0 | 0.956 | 0 |
| LOW | Hash-M-grow vs Hash-V-grow | B_used | 0 | 4 | 0 | 0 | 0 | 0 | 1.045 | 0 |
| LOW | Hash-M-grow vs Hash-V-grow | B_heap | 0 | 4 | 0 | 0 | 0 | 0 | 1.043 | 0 |
| LOW | Hash-V-grow vs Hash-V-fixed | creation | 0 | 0 | 4 | 0 | 0 | 0 | 0.722 | 0 |
| LOW | Hash-V-grow vs Hash-V-fixed | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.634 | 0 |
| LOW | Hash-V-grow vs Hash-V-fixed | B_used | 4 | 0 | 0 | 0 | 0 | 0 | 0.431 | 0 |
| LOW | Hash-V-grow vs Hash-V-fixed | B_heap | 4 | 0 | 0 | 0 | 0 | 0 | 0.445 | 0 |
| LOW | Hash-M-grow vs Hash-M-fixed | creation | 0 | 0 | 4 | 0 | 0 | 0 | 0.703 | 0 |
| LOW | Hash-M-grow vs Hash-M-fixed | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.740 | 0 |
| LOW | Hash-M-grow vs Hash-M-fixed | B_used | 4 | 0 | 0 | 0 | 0 | 0 | 0.405 | 0 |
| LOW | Hash-M-grow vs Hash-M-fixed | B_heap | 4 | 0 | 0 | 0 | 0 | 0 | 0.418 | 0 |
| LOW | Hash-V-grow-map vs Hash-V-grow | creation | 3 | 1 | 0 | 0 | 0 | 0 | 1.124 | 0 |
| LOW | Hash-V-grow-map vs Hash-V-grow | operation | 0 | 0 | 0 | 0 | 0 | 4 | 1.145 | 0 |
| LOW | Hash-V-grow-map vs Hash-V-grow | B_used | 0 | 0 | 0 | 0 | 0 | 4 | 1.000 | 0 |
| LOW | Hash-V-grow-map vs Hash-V-grow | B_heap | 0 | 0 | 0 | 0 | 0 | 4 | 1.000 | 0 |
| MID | Hash-V-fixed vs TC | creation | 0 | 0 | 4 | 0 | 0 | 0 | 0.645 | 0 |
| MID | Hash-V-fixed vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.544 | 0 |
| MID | Hash-V-fixed vs TC | B_used | 0 | 0 | 4 | 0 | 0 | 0 | 1.917 | 1 |
| MID | Hash-V-fixed vs TC | B_heap | 0 | 0 | 4 | 0 | 0 | 0 | 1.953 | 1 |
| MID | Hash-M-fixed vs TC | creation | 0 | 0 | 4 | 0 | 0 | 0 | 0.692 | 0 |
| MID | Hash-M-fixed vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.489 | 0 |
| MID | Hash-M-fixed vs TC | B_used | 0 | 0 | 4 | 0 | 0 | 0 | 2.083 | 4 |
| MID | Hash-M-fixed vs TC | B_heap | 0 | 0 | 4 | 0 | 0 | 0 | 2.120 | 4 |
| MID | Hash-V-grow vs TC | creation | 0 | 0 | 4 | 0 | 0 | 0 | 0.549 | 0 |
| MID | Hash-V-grow vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.374 | 0 |
| MID | Hash-V-grow vs TC | B_used | 4 | 0 | 0 | 0 | 0 | 0 | 0.500 | 0 |
| MID | Hash-V-grow vs TC | B_heap | 4 | 0 | 0 | 0 | 0 | 0 | 0.626 | 0 |
| MID | Hash-M-grow vs TC | creation | 0 | 0 | 4 | 0 | 0 | 0 | 0.566 | 0 |
| MID | Hash-M-grow vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.367 | 0 |
| MID | Hash-M-grow vs TC | B_used | 4 | 0 | 0 | 0 | 0 | 0 | 0.510 | 0 |
| MID | Hash-M-grow vs TC | B_heap | 4 | 0 | 0 | 0 | 0 | 0 | 0.634 | 0 |
| MID | Hash-M-fixed vs Hash-V-fixed | creation | 3 | 1 | 0 | 0 | 0 | 0 | 1.059 | 0 |
| MID | Hash-M-fixed vs Hash-V-fixed | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.898 | 0 |
| MID | Hash-M-fixed vs Hash-V-fixed | B_used | 0 | 3 | 1 | 0 | 0 | 0 | 1.087 | 0 |
| MID | Hash-M-fixed vs Hash-V-fixed | B_heap | 0 | 3 | 1 | 0 | 0 | 0 | 1.087 | 0 |
| MID | Hash-M-grow vs Hash-V-grow | creation | 0 | 4 | 0 | 0 | 0 | 0 | 1.022 | 0 |
| MID | Hash-M-grow vs Hash-V-grow | operation | 0 | 4 | 0 | 0 | 0 | 0 | 0.999 | 0 |
| MID | Hash-M-grow vs Hash-V-grow | B_used | 0 | 4 | 0 | 0 | 0 | 0 | 1.021 | 0 |
| MID | Hash-M-grow vs Hash-V-grow | B_heap | 0 | 4 | 0 | 0 | 0 | 0 | 1.018 | 0 |
| MID | Hash-V-grow vs Hash-V-fixed | creation | 0 | 0 | 4 | 0 | 0 | 0 | 0.854 | 0 |
| MID | Hash-V-grow vs Hash-V-fixed | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.706 | 0 |
| MID | Hash-V-grow vs Hash-V-fixed | B_used | 4 | 0 | 0 | 0 | 0 | 0 | 0.261 | 0 |
| MID | Hash-V-grow vs Hash-V-fixed | B_heap | 4 | 0 | 0 | 0 | 0 | 0 | 0.308 | 0 |
| MID | Hash-M-grow vs Hash-M-fixed | creation | 0 | 0 | 4 | 0 | 0 | 0 | 0.835 | 0 |
| MID | Hash-M-grow vs Hash-M-fixed | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.767 | 0 |
| MID | Hash-M-grow vs Hash-M-fixed | B_used | 4 | 0 | 0 | 0 | 0 | 0 | 0.245 | 0 |
| MID | Hash-M-grow vs Hash-M-fixed | B_heap | 4 | 0 | 0 | 0 | 0 | 0 | 0.280 | 0 |
| MID | Hash-V-grow-map vs Hash-V-grow | creation | 1 | 3 | 0 | 0 | 0 | 0 | 1.056 | 0 |
| MID | Hash-V-grow-map vs Hash-V-grow | operation | 0 | 0 | 0 | 0 | 0 | 4 | 1.102 | 0 |
| MID | Hash-V-grow-map vs Hash-V-grow | B_used | 0 | 0 | 0 | 0 | 0 | 4 | 1.000 | 0 |
| MID | Hash-V-grow-map vs Hash-V-grow | B_heap | 0 | 0 | 0 | 0 | 0 | 4 | 1.003 | 0 |
| HIGH | Hash-V-fixed vs TC | creation | 0 | 0 | 4 | 0 | 0 | 0 | 0.649 | 0 |
| HIGH | Hash-V-fixed vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.544 | 0 |
| HIGH | Hash-V-fixed vs TC | B_used | 0 | 0 | 4 | 0 | 0 | 0 | 1.917 | 1 |
| HIGH | Hash-V-fixed vs TC | B_heap | 0 | 0 | 4 | 0 | 0 | 0 | 1.953 | 1 |
| HIGH | Hash-M-fixed vs TC | creation | 0 | 0 | 4 | 0 | 0 | 0 | 0.668 | 0 |
| HIGH | Hash-M-fixed vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.496 | 0 |
| HIGH | Hash-M-fixed vs TC | B_used | 0 | 0 | 4 | 0 | 0 | 0 | 2.083 | 4 |
| HIGH | Hash-M-fixed vs TC | B_heap | 0 | 0 | 4 | 0 | 0 | 0 | 2.120 | 4 |
| HIGH | Hash-V-grow vs TC | creation | 0 | 0 | 4 | 0 | 0 | 0 | 0.585 | 0 |
| HIGH | Hash-V-grow vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.393 | 0 |
| HIGH | Hash-V-grow vs TC | B_used | 4 | 0 | 0 | 0 | 0 | 0 | 0.396 | 0 |
| HIGH | Hash-V-grow vs TC | B_heap | 4 | 0 | 0 | 0 | 0 | 0 | 0.428 | 0 |
| HIGH | Hash-M-grow vs TC | creation | 0 | 0 | 4 | 0 | 0 | 0 | 0.597 | 0 |
| HIGH | Hash-M-grow vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.397 | 0 |
| HIGH | Hash-M-grow vs TC | B_used | 4 | 0 | 0 | 0 | 0 | 0 | 0.398 | 0 |
| HIGH | Hash-M-grow vs TC | B_heap | 4 | 0 | 0 | 0 | 0 | 0 | 0.430 | 0 |
| HIGH | Hash-M-fixed vs Hash-V-fixed | creation | 2 | 2 | 0 | 0 | 0 | 0 | 1.055 | 0 |
| HIGH | Hash-M-fixed vs Hash-V-fixed | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.920 | 0 |
| HIGH | Hash-M-fixed vs Hash-V-fixed | B_used | 0 | 3 | 1 | 0 | 0 | 0 | 1.087 | 0 |
| HIGH | Hash-M-fixed vs Hash-V-fixed | B_heap | 0 | 3 | 1 | 0 | 0 | 0 | 1.087 | 0 |
| HIGH | Hash-M-grow vs Hash-V-grow | creation | 1 | 3 | 0 | 0 | 0 | 0 | 1.021 | 0 |
| HIGH | Hash-M-grow vs Hash-V-grow | operation | 0 | 4 | 0 | 0 | 0 | 0 | 0.997 | 0 |
| HIGH | Hash-M-grow vs Hash-V-grow | B_used | 0 | 4 | 0 | 0 | 0 | 0 | 1.007 | 0 |
| HIGH | Hash-M-grow vs Hash-V-grow | B_heap | 0 | 4 | 0 | 0 | 0 | 0 | 1.007 | 0 |
| HIGH | Hash-V-grow vs Hash-V-fixed | creation | 0 | 1 | 3 | 0 | 0 | 0 | 0.905 | 0 |
| HIGH | Hash-V-grow vs Hash-V-fixed | operation | 0 | 1 | 3 | 0 | 0 | 0 | 0.744 | 0 |
| HIGH | Hash-V-grow vs Hash-V-fixed | B_used | 4 | 0 | 0 | 0 | 0 | 0 | 0.207 | 0 |
| HIGH | Hash-V-grow vs Hash-V-fixed | B_heap | 4 | 0 | 0 | 0 | 0 | 0 | 0.224 | 0 |
| HIGH | Hash-M-grow vs Hash-M-fixed | creation | 0 | 0 | 4 | 0 | 0 | 0 | 0.892 | 0 |
| HIGH | Hash-M-grow vs Hash-M-fixed | operation | 0 | 1 | 3 | 0 | 0 | 0 | 0.802 | 0 |
| HIGH | Hash-M-grow vs Hash-M-fixed | B_used | 4 | 0 | 0 | 0 | 0 | 0 | 0.191 | 0 |
| HIGH | Hash-M-grow vs Hash-M-fixed | B_heap | 4 | 0 | 0 | 0 | 0 | 0 | 0.207 | 0 |
| HIGH | Hash-V-grow-map vs Hash-V-grow | creation | 1 | 3 | 0 | 0 | 0 | 0 | 1.019 | 0 |
| HIGH | Hash-V-grow-map vs Hash-V-grow | operation | 0 | 0 | 0 | 0 | 0 | 4 | 1.071 | 0 |
| HIGH | Hash-V-grow-map vs Hash-V-grow | B_used | 0 | 0 | 0 | 0 | 0 | 4 | 1.000 | 0 |
| HIGH | Hash-V-grow-map vs Hash-V-grow | B_heap | 0 | 0 | 0 | 0 | 0 | 4 | 1.003 | 0 |

### Size x sharing grid (ratios T(reference)/T(row); presented, not judged)

Hash-V-fixed against TC, creation:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.563 | 0.602 | 0.612 | 0.613 |
| 32 | 0.595 | 0.580 | 0.607 | 0.516 |
| 64 | 0.642 | 0.653 | 0.677 | 0.684 |
| 128 | 0.643 | 0.674 | 0.706 | 0.707 |

Hash-V-fixed against TC, operation:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.450 | 0.426 | 0.477 | 0.534 |
| 32 | 0.470 | 0.484 | 0.513 | 0.390 |
| 64 | 0.581 | 0.583 | 0.575 | 0.553 |
| 128 | 0.568 | 0.590 | 0.576 | 0.573 |

Hash-M-fixed against TC, creation:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.547 | 0.600 | 0.617 | 0.619 |
| 32 | 0.596 | 0.601 | 0.668 | 0.579 |
| 64 | 0.670 | 0.692 | 0.716 | 0.718 |
| 128 | 0.673 | 0.713 | 0.748 | 0.750 |

Hash-M-fixed against TC, operation:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.330 | 0.345 | 0.414 | 0.477 |
| 32 | 0.354 | 0.413 | 0.461 | 0.364 |
| 64 | 0.439 | 0.506 | 0.530 | 0.514 |
| 128 | 0.450 | 0.509 | 0.517 | 0.522 |

Hash-V-grow against TC, creation:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.165 | 0.381 | 0.499 | 0.526 |
| 32 | 0.299 | 0.354 | 0.505 | 0.449 |
| 64 | 0.374 | 0.529 | 0.593 | 0.643 |
| 128 | 0.406 | 0.577 | 0.646 | 0.694 |

Hash-V-grow against TC, operation:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.258 | 0.252 | 0.341 | 0.505 |
| 32 | 0.244 | 0.250 | 0.359 | 0.307 |
| 64 | 0.392 | 0.394 | 0.390 | 0.388 |
| 128 | 0.411 | 0.419 | 0.410 | 0.397 |

Hash-M-grow against TC, creation:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.170 | 0.381 | 0.514 | 0.536 |
| 32 | 0.297 | 0.353 | 0.533 | 0.478 |
| 64 | 0.373 | 0.533 | 0.599 | 0.658 |
| 128 | 0.406 | 0.578 | 0.653 | 0.696 |

Hash-M-grow against TC, operation:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.205 | 0.253 | 0.343 | 0.498 |
| 32 | 0.208 | 0.237 | 0.343 | 0.303 |
| 64 | 0.325 | 0.378 | 0.390 | 0.395 |
| 128 | 0.356 | 0.396 | 0.409 | 0.399 |

Hash-M-fixed against Hash-V-fixed, creation:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.972 | 0.997 | 1.007 | 1.009 |
| 32 | 1.002 | 1.036 | 1.102 | 1.123 |
| 64 | 1.043 | 1.061 | 1.058 | 1.049 |
| 128 | 1.046 | 1.058 | 1.060 | 1.060 |

Hash-M-fixed against Hash-V-fixed, operation:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.733 | 0.809 | 0.868 | 0.894 |
| 32 | 0.753 | 0.853 | 0.897 | 0.932 |
| 64 | 0.756 | 0.868 | 0.922 | 0.928 |
| 128 | 0.792 | 0.863 | 0.898 | 0.911 |

Hash-M-grow against Hash-V-grow, creation:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 1.029 | 1.000 | 1.032 | 1.018 |
| 32 | 0.993 | 0.997 | 1.054 | 1.064 |
| 64 | 0.998 | 1.008 | 1.010 | 1.023 |
| 128 | 0.999 | 1.003 | 1.011 | 1.003 |

Hash-M-grow against Hash-V-grow, operation:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.792 | 1.001 | 1.008 | 0.985 |
| 32 | 0.854 | 0.951 | 0.956 | 0.990 |
| 64 | 0.828 | 0.960 | 1.002 | 1.018 |
| 128 | 0.865 | 0.945 | 0.996 | 1.004 |

Hash-V-grow against Hash-V-fixed, creation:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.293 | 0.633 | 0.814 | 0.858 |
| 32 | 0.503 | 0.610 | 0.833 | 0.870 |
| 64 | 0.581 | 0.810 | 0.876 | 0.940 |
| 128 | 0.632 | 0.855 | 0.915 | 0.981 |

Hash-V-grow against Hash-V-fixed, operation:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.573 | 0.592 | 0.715 | 0.945 |
| 32 | 0.519 | 0.515 | 0.699 | 0.786 |
| 64 | 0.675 | 0.676 | 0.678 | 0.701 |
| 128 | 0.723 | 0.710 | 0.712 | 0.693 |

Hash-M-grow against Hash-M-fixed, creation:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.310 | 0.635 | 0.834 | 0.866 |
| 32 | 0.499 | 0.587 | 0.797 | 0.825 |
| 64 | 0.556 | 0.770 | 0.836 | 0.917 |
| 128 | 0.603 | 0.811 | 0.873 | 0.928 |

Hash-M-grow against Hash-M-fixed, operation:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.620 | 0.732 | 0.830 | 1.042 |
| 32 | 0.589 | 0.575 | 0.745 | 0.835 |
| 64 | 0.739 | 0.748 | 0.737 | 0.769 |
| 128 | 0.791 | 0.777 | 0.790 | 0.763 |

Hash-V-grow-map against Hash-V-grow, creation:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 1.485 | 1.177 | 1.053 | 1.021 |
| 32 | 1.212 | 1.216 | 1.066 | 1.065 |
| 64 | 1.187 | 1.072 | 1.060 | 1.017 |
| 128 | 1.157 | 1.051 | 1.013 | 1.002 |

Hash-V-grow-map against Hash-V-grow, operation:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 1.162 | 1.230 | 1.140 | 0.987 |
| 32 | 1.193 | 1.187 | 1.042 | 1.040 |
| 64 | 1.098 | 1.104 | 1.103 | 1.121 |
| 128 | 1.092 | 1.093 | 1.101 | 1.102 |

## The growth peak (grow rows; reported, no verdict)

The most bytes per cell at any growth: accounted (every matrix's cells and table, the old table included, plus the grown table (n), the slot map (r) in Hash-V-grow-map, and tau) and heap (`mallinfo2`, read inside the growth just before the old table is freed), over 3n^2 cells; and Hash-V-grow-map over Hash-V-grow.

| job | Hash-V-grow accounted, heap (growths) | Hash-M-grow accounted, heap (growths) | Hash-V-grow-map accounted, heap (growths) | map / lookup: accounted, heap |
| --- | --- | --- | --- | --- |
| 1 (n 16, rho 1) | 33.33, 36.90 (18) | 36.00, 39.65 (18) | 34.67, 36.90 (18) | 1.040, 1.000 |
| 2 (n 16, rho 4) | 17.33, 20.90 (14) | 18.00, 21.65 (14) | 18.67, 20.90 (14) | 1.077, 1.000 |
| 3 (n 16, rho 16) | 9.33, 11.48 (9) | 9.50, 11.54 (9) | 10.00, 11.54 (9) | 1.071, 1.005 |
| 4 (n 16, rho 64) | 5.17, 6.08 (4) | 5.25, 6.25 (4) | 5.25, 6.15 (4) | 1.016, 1.010 |
| 5 (n 32, rho 1) | 22.67, 23.56 (22) | 24.00, 24.91 (22) | 24.00, 24.90 (22) | 1.059, 1.057 |
| 6 (n 32, rho 4) | 17.33, 18.22 (20) | 18.00, 18.91 (20) | 18.67, 19.56 (20) | 1.077, 1.073 |
| 7 (n 32, rho 16) | 9.33, 10.21 (15) | 9.50, 10.40 (15) | 10.00, 10.90 (15) | 1.071, 1.067 |
| 8 (n 32, rho 64) | 6.33, 7.21 (10) | 6.38, 7.22 (10) | 6.67, 7.22 (10) | 1.053, 1.002 |
| 9 (n 64, rho 1) | 18.67, 18.89 (27) | 20.00, 20.22 (27) | 19.33, 19.56 (27) | 1.036, 1.036 |
| 10 (n 64, rho 4) | 10.67, 10.89 (23) | 11.00, 11.22 (23) | 11.33, 11.56 (23) | 1.063, 1.062 |
| 11 (n 64, rho 16) | 9.33, 9.55 (21) | 9.50, 9.72 (21) | 10.00, 10.22 (21) | 1.071, 1.070 |
| 12 (n 64, rho 64) | 8.33, 8.56 (17) | 8.38, 8.60 (17) | 9.00, 9.22 (17) | 1.080, 1.078 |
| 13 (n 128, rho 1) | 22.67, 22.72 (33) | 25.33, 25.39 (33) | 24.00, 24.09 (33) | 1.059, 1.060 |
| 14 (n 128, rho 4) | 14.67, 14.72 (29) | 15.33, 15.39 (29) | 16.00, 16.09 (29) | 1.091, 1.093 |
| 15 (n 128, rho 16) | 12.67, 12.72 (25) | 12.83, 12.89 (25) | 14.00, 14.10 (25) | 1.105, 1.108 |
| 16 (n 128, rho 64) | 12.33, 12.39 (23) | 12.42, 12.47 (23) | 13.67, 13.76 (23) | 1.108, 1.111 |

## tau's build time (M rows; reported, no verdict)

The median over rounds of tau's build time per run, and its share of the operation part (the median of the per-round shares). tau is built, used and freed inside the operation part (spec section 8).

| job | Hash-M-fixed tau ns (share) | Hash-M-grow tau ns (share) |
| --- | --- | --- |
| 1 (n 16, rho 1) | 3550 (0.2639) | 4760 (0.2198) |
| 2 (n 16, rho 4) | 2390 (0.2012) | 880 (0.0548) |
| 3 (n 16, rho 16) | 1632 (0.1515) | 268 (0.0208) |
| 4 (n 16, rho 64) | 1296 (0.1128) | 110 (0.0100) |
| 5 (n 32, rho 1) | 18701 (0.2661) | 18157 (0.1517) |
| 6 (n 32, rho 4) | 9432 (0.1566) | 5857 (0.0563) |
| 7 (n 32, rho 16) | 6091 (0.1074) | 1611 (0.0208) |
| 8 (n 32, rho 64) | 4929 (0.1045) | 386 (0.0068) |
| 9 (n 64, rho 1) | 76538 (0.2452) | 75279 (0.1779) |
| 10 (n 64, rho 4) | 36114 (0.1346) | 19868 (0.0553) |
| 11 (n 64, rho 16) | 23105 (0.0913) | 6471 (0.0189) |
| 12 (n 64, rho 64) | 19417 (0.0710) | 1836 (0.0052) |
| 13 (n 128, rho 1) | 240621 (0.1873) | 205320 (0.1274) |
| 14 (n 128, rho 4) | 136089 (0.1180) | 75044 (0.0496) |
| 15 (n 128, rho 16) | 94788 (0.0847) | 20131 (0.0142) |
| 16 (n 128, rho 64) | 78590 (0.0740) | 6387 (0.0046) |

## Stores, per job (rule-m1 item 13)

From the counting runs. Fixed: Hash-V-fixed (Hash-M-fixed has the same stores). Grow: Hash-V-grow (Hash-M-grow the same; Hash-V-grow-map the same but no rewrite probe). W counts 0 and 1. Probes per store over every whole-number store (0 and 1 found with no probe).

| job | sizing | store | when | W | T | fill | w | w_h | stores (found / new) | probes per store | longest | widenings (slots copied) | growths | reinserted (probes) | fields rewritten | rewrite probes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | fixed | A | after creation | 286 | 1024 | 27.9% | 64 | 16 | 228 / 284 | 1.08 | 6 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 1 | fixed | A | end | 286 | 1024 | 27.9% | 64 | 16 | 228 / 284 | 1.08 | 6 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 1 | fixed | B | after creation | 285 | 1024 | 27.8% | 64 | 16 | 229 / 283 | 1.04 | 5 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 1 | fixed | B | end | 285 | 1024 | 27.8% | 64 | 16 | 229 / 283 | 1.04 | 5 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 1 | fixed | C | after creation | 2 | 1024 | 0.2% | 8 | 16 | 0 / 0 | n/a | 0 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 1 | fixed | C | end | 286 | 1024 | 27.9% | 32 | 16 | 228 / 284 | 1.10 | 5 | 2 (2048) | 0 | 0 (0) | 0 | 0 |
| 1 | grow | A | after creation | 286 | 1024 | 27.9% | 64 | 16 | 228 / 284 | 1.54 | 10 | 0 (0) | 6 | 492 (600) | 796 | 888 |
| 1 | grow | A | end | 286 | 1024 | 27.9% | 64 | 16 | 228 / 284 | 1.54 | 10 | 0 (0) | 6 | 492 (600) | 796 | 888 |
| 1 | grow | B | after creation | 285 | 1024 | 27.8% | 64 | 16 | 229 / 283 | 1.38 | 13 | 0 (0) | 6 | 492 (562) | 794 | 826 |
| 1 | grow | B | end | 285 | 1024 | 27.8% | 64 | 16 | 229 / 283 | 1.38 | 13 | 0 (0) | 6 | 492 (562) | 794 | 826 |
| 1 | grow | C | after creation | 2 | 16 | 12.5% | 8 | 16 | 0 / 0 | n/a | 0 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 1 | grow | C | end | 286 | 1024 | 27.9% | 32 | 16 | 228 / 284 | 1.41 | 13 | 2 (32) | 6 | 492 (575) | 782 | 867 |
| 2 | fixed | A | after creation | 88 | 1024 | 8.6% | 64 | 16 | 426 / 86 | 0.95 | 2 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 2 | fixed | A | end | 88 | 1024 | 8.6% | 64 | 16 | 426 / 86 | 0.95 | 2 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 2 | fixed | B | after creation | 87 | 1024 | 8.5% | 64 | 16 | 427 / 85 | 0.98 | 4 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 2 | fixed | B | end | 87 | 1024 | 8.5% | 64 | 16 | 427 / 85 | 0.98 | 4 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 2 | fixed | C | after creation | 2 | 1024 | 0.2% | 8 | 16 | 0 / 0 | n/a | 0 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 2 | fixed | C | end | 282 | 1024 | 27.5% | 32 | 16 | 232 / 280 | 1.08 | 5 | 2 (2048) | 0 | 0 (0) | 0 | 0 |
| 2 | grow | A | after creation | 88 | 256 | 34.4% | 64 | 16 | 426 / 86 | 1.12 | 12 | 0 (0) | 4 | 112 (125) | 195 | 198 |
| 2 | grow | A | end | 88 | 256 | 34.4% | 64 | 16 | 426 / 86 | 1.12 | 12 | 0 (0) | 4 | 112 (125) | 195 | 198 |
| 2 | grow | B | after creation | 87 | 256 | 34.0% | 64 | 16 | 427 / 85 | 1.15 | 8 | 0 (0) | 4 | 112 (128) | 196 | 203 |
| 2 | grow | B | end | 87 | 256 | 34.0% | 64 | 16 | 427 / 85 | 1.15 | 8 | 0 (0) | 4 | 112 (128) | 196 | 203 |
| 2 | grow | C | after creation | 2 | 16 | 12.5% | 8 | 16 | 0 / 0 | n/a | 0 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 2 | grow | C | end | 282 | 1024 | 27.5% | 32 | 16 | 232 / 280 | 1.40 | 10 | 2 (32) | 6 | 492 (566) | 785 | 853 |
| 3 | fixed | A | after creation | 28 | 1024 | 2.7% | 64 | 16 | 486 / 26 | 0.97 | 1 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 3 | fixed | A | end | 28 | 1024 | 2.7% | 64 | 16 | 486 / 26 | 0.97 | 1 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 3 | fixed | B | after creation | 28 | 1024 | 2.7% | 64 | 16 | 486 / 26 | 0.93 | 1 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 3 | fixed | B | end | 28 | 1024 | 2.7% | 64 | 16 | 486 / 26 | 0.93 | 1 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 3 | fixed | C | after creation | 2 | 1024 | 0.2% | 8 | 16 | 0 / 0 | n/a | 0 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 3 | fixed | C | end | 182 | 1024 | 17.8% | 32 | 16 | 332 / 180 | 1.09 | 5 | 1 (1024) | 0 | 0 (0) | 0 | 0 |
| 3 | grow | A | after creation | 28 | 64 | 43.8% | 64 | 16 | 486 / 26 | 1.51 | 10 | 0 (0) | 2 | 20 (22) | 34 | 37 |
| 3 | grow | A | end | 28 | 64 | 43.8% | 64 | 16 | 486 / 26 | 1.51 | 10 | 0 (0) | 2 | 20 (22) | 34 | 37 |
| 3 | grow | B | after creation | 28 | 64 | 43.8% | 64 | 16 | 486 / 26 | 1.16 | 10 | 0 (0) | 2 | 20 (24) | 28 | 30 |
| 3 | grow | B | end | 28 | 64 | 43.8% | 64 | 16 | 486 / 26 | 1.16 | 10 | 0 (0) | 2 | 20 (24) | 28 | 30 |
| 3 | grow | C | after creation | 2 | 16 | 12.5% | 8 | 16 | 0 / 0 | n/a | 0 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 3 | grow | C | end | 182 | 512 | 35.5% | 32 | 16 | 332 / 180 | 1.30 | 8 | 1 (16) | 5 | 238 (281) | 414 | 462 |
| 4 | fixed | A | after creation | 9 | 1024 | 0.9% | 64 | 16 | 505 / 7 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 4 | fixed | A | end | 9 | 1024 | 0.9% | 64 | 16 | 505 / 7 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 4 | fixed | B | after creation | 10 | 1024 | 1.0% | 64 | 16 | 504 / 8 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 4 | fixed | B | end | 10 | 1024 | 1.0% | 64 | 16 | 504 / 8 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 4 | fixed | C | after creation | 2 | 1024 | 0.2% | 8 | 16 | 0 / 0 | n/a | 0 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 4 | fixed | C | end | 28 | 1024 | 2.7% | 32 | 16 | 486 / 26 | 1.06 | 2 | 1 (1024) | 0 | 0 (0) | 0 | 0 |
| 4 | grow | A | after creation | 9 | 32 | 28.1% | 64 | 16 | 505 / 7 | 1.80 | 10 | 0 (0) | 1 | 6 (9) | 13 | 19 |
| 4 | grow | A | end | 9 | 32 | 28.1% | 64 | 16 | 505 / 7 | 1.80 | 10 | 0 (0) | 1 | 6 (9) | 13 | 19 |
| 4 | grow | B | after creation | 10 | 32 | 31.2% | 64 | 16 | 504 / 8 | 1.11 | 2 | 0 (0) | 1 | 6 (6) | 16 | 16 |
| 4 | grow | B | end | 10 | 32 | 31.2% | 64 | 16 | 504 / 8 | 1.11 | 2 | 0 (0) | 1 | 6 (6) | 16 | 16 |
| 4 | grow | C | after creation | 2 | 16 | 12.5% | 8 | 16 | 0 / 0 | n/a | 0 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 4 | grow | C | end | 28 | 64 | 43.8% | 32 | 16 | 486 / 26 | 1.47 | 6 | 1 (16) | 2 | 20 (27) | 23 | 30 |
| 5 | fixed | A | after creation | 1023 | 4096 | 25.0% | 64 | 16 | 1027 / 1021 | 1.02 | 6 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 5 | fixed | A | end | 1023 | 4096 | 25.0% | 64 | 16 | 1027 / 1021 | 1.02 | 6 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 5 | fixed | B | after creation | 1013 | 4096 | 24.7% | 64 | 16 | 1037 / 1011 | 1.00 | 5 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 5 | fixed | B | end | 1013 | 4096 | 24.7% | 64 | 16 | 1037 / 1011 | 1.00 | 5 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 5 | fixed | C | after creation | 2 | 4096 | 0.0% | 8 | 16 | 0 / 0 | n/a | 0 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 5 | fixed | C | end | 1045 | 4096 | 25.5% | 32 | 16 | 1005 / 1043 | 1.11 | 10 | 2 (8192) | 0 | 0 (0) | 0 | 0 |
| 5 | grow | A | after creation | 1023 | 2048 | 50.0% | 64 | 16 | 1027 / 1021 | 1.37 | 13 | 0 (0) | 7 | 1002 (1213) | 1775 | 1900 |
| 5 | grow | A | end | 1023 | 2048 | 50.0% | 64 | 16 | 1027 / 1021 | 1.37 | 13 | 0 (0) | 7 | 1002 (1213) | 1775 | 1900 |
| 5 | grow | B | after creation | 1013 | 2048 | 49.5% | 64 | 16 | 1037 / 1011 | 1.34 | 12 | 0 (0) | 7 | 1002 (1142) | 1800 | 1843 |
| 5 | grow | B | end | 1013 | 2048 | 49.5% | 64 | 16 | 1037 / 1011 | 1.34 | 12 | 0 (0) | 7 | 1002 (1142) | 1800 | 1843 |
| 5 | grow | C | after creation | 2 | 16 | 12.5% | 8 | 16 | 0 / 0 | n/a | 0 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 5 | grow | C | end | 1045 | 4096 | 25.5% | 32 | 16 | 1005 / 1043 | 1.43 | 16 | 2 (32) | 8 | 2024 (2386) | 3753 | 4170 |
| 6 | fixed | A | after creation | 284 | 4096 | 6.9% | 64 | 16 | 1766 / 282 | 0.95 | 2 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 6 | fixed | A | end | 284 | 4096 | 6.9% | 64 | 16 | 1766 / 282 | 0.95 | 2 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 6 | fixed | B | after creation | 285 | 4096 | 7.0% | 64 | 16 | 1765 / 283 | 0.98 | 4 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 6 | fixed | B | end | 285 | 4096 | 7.0% | 64 | 16 | 1765 / 283 | 0.98 | 4 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 6 | fixed | C | after creation | 2 | 4096 | 0.0% | 8 | 16 | 0 / 0 | n/a | 0 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 6 | fixed | C | end | 1032 | 4096 | 25.2% | 32 | 16 | 1018 / 1030 | 1.09 | 7 | 2 (8192) | 0 | 0 (0) | 0 | 0 |
| 6 | grow | A | after creation | 284 | 1024 | 27.7% | 64 | 16 | 1766 / 282 | 1.16 | 9 | 0 (0) | 6 | 492 (561) | 1436 | 1453 |
| 6 | grow | A | end | 284 | 1024 | 27.7% | 64 | 16 | 1766 / 282 | 1.16 | 9 | 0 (0) | 6 | 492 (561) | 1436 | 1453 |
| 6 | grow | B | after creation | 285 | 1024 | 27.8% | 64 | 16 | 1765 / 283 | 1.24 | 14 | 0 (0) | 6 | 492 (585) | 1453 | 1524 |
| 6 | grow | B | end | 285 | 1024 | 27.8% | 64 | 16 | 1765 / 283 | 1.24 | 14 | 0 (0) | 6 | 492 (585) | 1453 | 1524 |
| 6 | grow | C | after creation | 2 | 16 | 12.5% | 8 | 16 | 0 / 0 | n/a | 0 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 6 | grow | C | end | 1032 | 4096 | 25.2% | 32 | 16 | 1018 / 1030 | 1.43 | 17 | 2 (32) | 8 | 2024 (2359) | 3791 | 4119 |
| 7 | fixed | A | after creation | 92 | 4096 | 2.2% | 64 | 16 | 1958 / 90 | 0.96 | 2 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 7 | fixed | A | end | 92 | 4096 | 2.2% | 64 | 16 | 1958 / 90 | 0.96 | 2 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 7 | fixed | B | after creation | 86 | 4096 | 2.1% | 64 | 16 | 1964 / 84 | 0.97 | 1 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 7 | fixed | B | end | 86 | 4096 | 2.1% | 64 | 16 | 1964 / 84 | 0.97 | 1 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 7 | fixed | C | after creation | 2 | 4096 | 0.0% | 8 | 16 | 0 / 0 | n/a | 0 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 7 | fixed | C | end | 928 | 4096 | 22.7% | 32 | 16 | 1122 / 926 | 1.07 | 5 | 1 (4096) | 0 | 0 (0) | 0 | 0 |
| 7 | grow | A | after creation | 92 | 256 | 35.9% | 64 | 16 | 1958 / 90 | 1.08 | 4 | 0 (0) | 4 | 112 (123) | 206 | 212 |
| 7 | grow | A | end | 92 | 256 | 35.9% | 64 | 16 | 1958 / 90 | 1.08 | 4 | 0 (0) | 4 | 112 (123) | 206 | 212 |
| 7 | grow | B | after creation | 86 | 256 | 33.6% | 64 | 16 | 1964 / 84 | 1.16 | 9 | 0 (0) | 4 | 112 (131) | 182 | 198 |
| 7 | grow | B | end | 86 | 256 | 33.6% | 64 | 16 | 1964 / 84 | 1.16 | 9 | 0 (0) | 4 | 112 (131) | 182 | 198 |
| 7 | grow | C | after creation | 2 | 16 | 12.5% | 8 | 16 | 0 / 0 | n/a | 0 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 7 | grow | C | end | 928 | 2048 | 45.3% | 32 | 16 | 1122 / 926 | 1.36 | 13 | 1 (16) | 7 | 1002 (1149) | 1837 | 1966 |
| 8 | fixed | A | after creation | 28 | 4096 | 0.7% | 64 | 16 | 2022 / 26 | 0.88 | 1 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 8 | fixed | A | end | 28 | 4096 | 0.7% | 64 | 16 | 2022 / 26 | 0.88 | 1 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 8 | fixed | B | after creation | 28 | 4096 | 0.7% | 64 | 16 | 2022 / 26 | 0.93 | 1 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 8 | fixed | B | end | 28 | 4096 | 0.7% | 64 | 16 | 2022 / 26 | 0.93 | 1 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 8 | fixed | C | after creation | 2 | 4096 | 0.0% | 8 | 16 | 0 / 0 | n/a | 0 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 8 | fixed | C | end | 281 | 4096 | 6.9% | 32 | 16 | 1769 / 279 | 1.00 | 3 | 1 (4096) | 0 | 0 (0) | 0 | 0 |
| 8 | grow | A | after creation | 28 | 64 | 43.8% | 64 | 16 | 2022 / 26 | 1.20 | 4 | 0 (0) | 2 | 20 (22) | 46 | 37 |
| 8 | grow | A | end | 28 | 64 | 43.8% | 64 | 16 | 2022 / 26 | 1.20 | 4 | 0 (0) | 2 | 20 (22) | 46 | 37 |
| 8 | grow | B | after creation | 28 | 64 | 43.8% | 64 | 16 | 2022 / 26 | 1.21 | 6 | 0 (0) | 2 | 20 (29) | 41 | 50 |
| 8 | grow | B | end | 28 | 64 | 43.8% | 64 | 16 | 2022 / 26 | 1.21 | 6 | 0 (0) | 2 | 20 (29) | 41 | 50 |
| 8 | grow | C | after creation | 2 | 16 | 12.5% | 8 | 16 | 0 / 0 | n/a | 0 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 8 | grow | C | end | 281 | 1024 | 27.4% | 32 | 16 | 1769 / 279 | 1.17 | 12 | 1 (16) | 6 | 492 (554) | 1589 | 1667 |
| 9 | fixed | A | after creation | 3560 | 16384 | 21.7% | 64 | 16 | 4634 / 3558 | 0.97 | 4 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 9 | fixed | A | end | 3560 | 16384 | 21.7% | 64 | 16 | 4634 / 3558 | 0.97 | 4 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 9 | fixed | B | after creation | 3544 | 16384 | 21.6% | 64 | 16 | 4650 / 3542 | 0.97 | 5 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 9 | fixed | B | end | 3544 | 16384 | 21.6% | 64 | 16 | 4650 / 3542 | 0.97 | 5 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 9 | fixed | C | after creation | 2 | 16384 | 0.0% | 8 | 16 | 0 / 0 | n/a | 0 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 9 | fixed | C | end | 4014 | 16384 | 24.5% | 32 | 16 | 4180 / 4012 | 1.06 | 6 | 1 (16384) | 0 | 0 (0) | 0 | 0 |
| 9 | grow | A | after creation | 3560 | 8192 | 43.5% | 64 | 16 | 4634 / 3558 | 1.21 | 18 | 0 (0) | 9 | 4070 (4522) | 8271 | 8282 |
| 9 | grow | A | end | 3560 | 8192 | 43.5% | 64 | 16 | 4634 / 3558 | 1.21 | 18 | 0 (0) | 9 | 4070 (4522) | 8271 | 8282 |
| 9 | grow | B | after creation | 3544 | 8192 | 43.3% | 64 | 16 | 4650 / 3542 | 1.21 | 14 | 0 (0) | 9 | 4070 (4507) | 8292 | 8353 |
| 9 | grow | B | end | 3544 | 8192 | 43.3% | 64 | 16 | 4650 / 3542 | 1.21 | 14 | 0 (0) | 9 | 4070 (4507) | 8292 | 8353 |
| 9 | grow | C | after creation | 2 | 16 | 12.5% | 8 | 16 | 0 / 0 | n/a | 0 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 9 | grow | C | end | 4014 | 8192 | 49.0% | 32 | 16 | 4180 / 4012 | 1.40 | 23 | 1 (16) | 9 | 4070 (4734) | 7830 | 8393 |
| 10 | fixed | A | after creation | 1014 | 16384 | 6.2% | 64 | 16 | 7180 / 1012 | 0.95 | 2 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 10 | fixed | A | end | 1014 | 16384 | 6.2% | 64 | 16 | 7180 / 1012 | 0.95 | 2 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 10 | fixed | B | after creation | 1008 | 16384 | 6.2% | 64 | 16 | 7186 / 1006 | 0.95 | 3 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 10 | fixed | B | end | 1008 | 16384 | 6.2% | 64 | 16 | 7186 / 1006 | 0.95 | 3 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 10 | fixed | C | after creation | 2 | 16384 | 0.0% | 8 | 16 | 0 / 0 | n/a | 0 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 10 | fixed | C | end | 3989 | 16384 | 24.3% | 32 | 16 | 4205 / 3987 | 1.05 | 6 | 1 (16384) | 0 | 0 (0) | 0 | 0 |
| 10 | grow | A | after creation | 1014 | 2048 | 49.5% | 64 | 16 | 7180 / 1012 | 1.16 | 15 | 0 (0) | 7 | 1002 (1172) | 2278 | 2401 |
| 10 | grow | A | end | 1014 | 2048 | 49.5% | 64 | 16 | 7180 / 1012 | 1.16 | 15 | 0 (0) | 7 | 1002 (1172) | 2278 | 2401 |
| 10 | grow | B | after creation | 1008 | 2048 | 49.2% | 64 | 16 | 7186 / 1006 | 1.18 | 13 | 0 (0) | 7 | 1002 (1142) | 2155 | 2231 |
| 10 | grow | B | end | 1008 | 2048 | 49.2% | 64 | 16 | 7186 / 1006 | 1.18 | 13 | 0 (0) | 7 | 1002 (1142) | 2155 | 2231 |
| 10 | grow | C | after creation | 2 | 16 | 12.5% | 8 | 16 | 0 / 0 | n/a | 0 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 10 | grow | C | end | 3989 | 8192 | 48.7% | 32 | 16 | 4205 / 3987 | 1.36 | 29 | 1 (16) | 9 | 4070 (4672) | 7851 | 8388 |
| 11 | fixed | A | after creation | 280 | 16384 | 1.7% | 64 | 16 | 7914 / 278 | 0.93 | 1 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 11 | fixed | A | end | 280 | 16384 | 1.7% | 64 | 16 | 7914 / 278 | 0.93 | 1 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 11 | fixed | B | after creation | 282 | 16384 | 1.7% | 64 | 16 | 7912 / 280 | 0.94 | 2 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 11 | fixed | B | end | 282 | 16384 | 1.7% | 64 | 16 | 7912 / 280 | 0.94 | 2 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 11 | fixed | C | after creation | 2 | 16384 | 0.0% | 8 | 16 | 0 / 0 | n/a | 0 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 11 | fixed | C | end | 3895 | 16384 | 23.8% | 32 | 16 | 4299 / 3893 | 1.06 | 7 | 1 (16384) | 0 | 0 (0) | 0 | 0 |
| 11 | grow | A | after creation | 280 | 1024 | 27.3% | 64 | 16 | 7914 / 278 | 1.06 | 10 | 0 (0) | 6 | 492 (551) | 1636 | 1629 |
| 11 | grow | A | end | 280 | 1024 | 27.3% | 64 | 16 | 7914 / 278 | 1.06 | 10 | 0 (0) | 6 | 492 (551) | 1636 | 1629 |
| 11 | grow | B | after creation | 282 | 1024 | 27.5% | 64 | 16 | 7912 / 280 | 1.04 | 9 | 0 (0) | 6 | 492 (544) | 1506 | 1499 |
| 11 | grow | B | end | 282 | 1024 | 27.5% | 64 | 16 | 7912 / 280 | 1.04 | 9 | 0 (0) | 6 | 492 (544) | 1506 | 1499 |
| 11 | grow | C | after creation | 2 | 16 | 12.5% | 8 | 16 | 0 / 0 | n/a | 0 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 11 | grow | C | end | 3895 | 8192 | 47.5% | 32 | 16 | 4299 / 3893 | 1.37 | 31 | 1 (16) | 9 | 4070 (4653) | 7899 | 8412 |
| 12 | fixed | A | after creation | 88 | 16384 | 0.5% | 64 | 16 | 8106 / 86 | 0.97 | 1 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 12 | fixed | A | end | 88 | 16384 | 0.5% | 64 | 16 | 8106 / 86 | 0.97 | 1 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 12 | fixed | B | after creation | 90 | 16384 | 0.5% | 64 | 16 | 8104 / 88 | 0.93 | 1 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 12 | fixed | B | end | 90 | 16384 | 0.5% | 64 | 16 | 8104 / 88 | 0.93 | 1 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 12 | fixed | C | after creation | 2 | 16384 | 0.0% | 8 | 16 | 0 / 0 | n/a | 0 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 12 | fixed | C | end | 2589 | 16384 | 15.8% | 32 | 16 | 5605 / 2587 | 1.03 | 5 | 2 (32768) | 0 | 0 (0) | 0 | 0 |
| 12 | grow | A | after creation | 88 | 256 | 34.4% | 64 | 16 | 8106 / 86 | 1.11 | 5 | 0 (0) | 4 | 112 (130) | 196 | 214 |
| 12 | grow | A | end | 88 | 256 | 34.4% | 64 | 16 | 8106 / 86 | 1.11 | 5 | 0 (0) | 4 | 112 (130) | 196 | 214 |
| 12 | grow | B | after creation | 90 | 256 | 35.2% | 64 | 16 | 8104 / 88 | 1.12 | 9 | 0 (0) | 4 | 112 (128) | 197 | 199 |
| 12 | grow | B | end | 90 | 256 | 35.2% | 64 | 16 | 8104 / 88 | 1.12 | 9 | 0 (0) | 4 | 112 (128) | 197 | 199 |
| 12 | grow | C | after creation | 2 | 16 | 12.5% | 8 | 16 | 0 / 0 | n/a | 0 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 12 | grow | C | end | 2589 | 8192 | 31.6% | 32 | 16 | 5605 / 2587 | 1.28 | 20 | 2 (32) | 9 | 4070 (4692) | 9713 | 10416 |
| 13 | fixed | A | after creation | 10220 | 65536 | 15.6% | 64 | 32 | 22550 / 10218 | 0.95 | 2 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 13 | fixed | A | end | 10220 | 65536 | 15.6% | 64 | 32 | 22550 / 10218 | 0.95 | 2 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 13 | fixed | B | after creation | 10124 | 65536 | 15.4% | 64 | 32 | 22646 / 10122 | 0.94 | 2 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 13 | fixed | B | end | 10124 | 65536 | 15.4% | 64 | 32 | 22646 / 10122 | 0.94 | 2 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 13 | fixed | C | after creation | 2 | 65536 | 0.0% | 8 | 32 | 0 / 0 | n/a | 0 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 13 | fixed | C | end | 14719 | 65536 | 22.5% | 32 | 32 | 18051 / 14717 | 1.03 | 7 | 1 (65536) | 0 | 0 (0) | 0 | 0 |
| 13 | grow | A | after creation | 10220 | 32768 | 31.2% | 64 | 32 | 22550 / 10218 | 1.08 | 13 | 0 (0) | 11 | 16354 (17326) | 41676 | 40401 |
| 13 | grow | A | end | 10220 | 32768 | 31.2% | 64 | 32 | 22550 / 10218 | 1.08 | 13 | 0 (0) | 11 | 16354 (17326) | 41676 | 40401 |
| 13 | grow | B | after creation | 10124 | 32768 | 30.9% | 64 | 32 | 22646 / 10122 | 1.09 | 15 | 0 (0) | 11 | 16354 (17388) | 41918 | 40648 |
| 13 | grow | B | end | 10124 | 32768 | 30.9% | 64 | 32 | 22646 / 10122 | 1.09 | 15 | 0 (0) | 11 | 16354 (17388) | 41918 | 40648 |
| 13 | grow | C | after creation | 2 | 16 | 12.5% | 8 | 32 | 0 / 0 | n/a | 0 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 13 | grow | C | end | 14719 | 32768 | 44.9% | 32 | 32 | 18051 / 14717 | 1.28 | 19 | 1 (16) | 11 | 16354 (18515) | 33703 | 35510 |
| 14 | fixed | A | after creation | 3541 | 65536 | 5.4% | 64 | 32 | 29229 / 3539 | 0.95 | 1 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 14 | fixed | A | end | 3541 | 65536 | 5.4% | 64 | 32 | 29229 / 3539 | 0.95 | 1 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 14 | fixed | B | after creation | 3562 | 65536 | 5.4% | 64 | 32 | 29208 / 3560 | 0.94 | 1 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 14 | fixed | B | end | 3562 | 65536 | 5.4% | 64 | 32 | 29208 / 3560 | 0.94 | 1 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 14 | fixed | C | after creation | 2 | 65536 | 0.0% | 8 | 32 | 0 / 0 | n/a | 0 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 14 | fixed | C | end | 14574 | 65536 | 22.2% | 32 | 32 | 18196 / 14572 | 1.03 | 6 | 1 (65536) | 0 | 0 (0) | 0 | 0 |
| 14 | grow | A | after creation | 3541 | 8192 | 43.2% | 64 | 32 | 29229 / 3539 | 1.10 | 18 | 0 (0) | 9 | 4070 (4518) | 10399 | 10637 |
| 14 | grow | A | end | 3541 | 8192 | 43.2% | 64 | 32 | 29229 / 3539 | 1.10 | 18 | 0 (0) | 9 | 4070 (4518) | 10399 | 10637 |
| 14 | grow | B | after creation | 3562 | 8192 | 43.5% | 64 | 32 | 29208 / 3560 | 1.09 | 16 | 0 (0) | 9 | 4070 (4624) | 10310 | 10503 |
| 14 | grow | B | end | 3562 | 8192 | 43.5% | 64 | 32 | 29208 / 3560 | 1.09 | 16 | 0 (0) | 9 | 4070 (4624) | 10310 | 10503 |
| 14 | grow | C | after creation | 2 | 16 | 12.5% | 8 | 32 | 0 / 0 | n/a | 0 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 14 | grow | C | end | 14574 | 32768 | 44.5% | 32 | 32 | 18196 / 14572 | 1.28 | 21 | 1 (16) | 11 | 16354 (18616) | 33770 | 35772 |
| 15 | fixed | A | after creation | 1012 | 65536 | 1.5% | 64 | 32 | 31758 / 1010 | 0.95 | 1 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 15 | fixed | A | end | 1012 | 65536 | 1.5% | 64 | 32 | 31758 / 1010 | 0.95 | 1 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 15 | fixed | B | after creation | 1017 | 65536 | 1.6% | 64 | 32 | 31753 / 1015 | 0.94 | 2 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 15 | fixed | B | end | 1017 | 65536 | 1.6% | 64 | 32 | 31753 / 1015 | 0.94 | 2 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 15 | fixed | C | after creation | 2 | 65536 | 0.0% | 8 | 32 | 0 / 0 | n/a | 0 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 15 | fixed | C | end | 14562 | 65536 | 22.2% | 32 | 32 | 18208 / 14560 | 1.03 | 6 | 2 (131072) | 0 | 0 (0) | 0 | 0 |
| 15 | grow | A | after creation | 1012 | 2048 | 49.4% | 64 | 32 | 31758 / 1010 | 1.17 | 14 | 0 (0) | 7 | 1002 (1123) | 2201 | 2243 |
| 15 | grow | A | end | 1012 | 2048 | 49.4% | 64 | 32 | 31758 / 1010 | 1.17 | 14 | 0 (0) | 7 | 1002 (1123) | 2201 | 2243 |
| 15 | grow | B | after creation | 1017 | 2048 | 49.7% | 64 | 32 | 31753 / 1015 | 1.15 | 13 | 0 (0) | 7 | 1002 (1134) | 2259 | 2267 |
| 15 | grow | B | end | 1017 | 2048 | 49.7% | 64 | 32 | 31753 / 1015 | 1.15 | 13 | 0 (0) | 7 | 1002 (1134) | 2259 | 2267 |
| 15 | grow | C | after creation | 2 | 16 | 12.5% | 8 | 32 | 0 / 0 | n/a | 0 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 15 | grow | C | end | 14562 | 32768 | 44.4% | 32 | 32 | 18208 / 14560 | 1.30 | 18 | 2 (48) | 11 | 16354 (18716) | 33680 | 35782 |
| 16 | fixed | A | after creation | 282 | 65536 | 0.4% | 64 | 32 | 32488 / 280 | 0.92 | 1 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 16 | fixed | A | end | 282 | 65536 | 0.4% | 64 | 32 | 32488 / 280 | 0.92 | 1 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 16 | fixed | B | after creation | 284 | 65536 | 0.4% | 64 | 32 | 32486 / 282 | 0.93 | 1 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 16 | fixed | B | end | 284 | 65536 | 0.4% | 64 | 32 | 32486 / 282 | 0.93 | 1 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 16 | fixed | C | after creation | 2 | 65536 | 0.0% | 8 | 32 | 0 / 0 | n/a | 0 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 16 | fixed | C | end | 13123 | 65536 | 20.0% | 32 | 32 | 19647 / 13121 | 1.02 | 7 | 1 (65536) | 0 | 0 (0) | 0 | 0 |
| 16 | grow | A | after creation | 282 | 1024 | 27.5% | 64 | 32 | 32488 / 280 | 1.02 | 10 | 0 (0) | 6 | 492 (544) | 1575 | 1539 |
| 16 | grow | A | end | 282 | 1024 | 27.5% | 64 | 32 | 32488 / 280 | 1.02 | 10 | 0 (0) | 6 | 492 (544) | 1575 | 1539 |
| 16 | grow | B | after creation | 284 | 1024 | 27.7% | 64 | 32 | 32486 / 282 | 1.04 | 12 | 0 (0) | 6 | 492 (569) | 1319 | 1338 |
| 16 | grow | B | end | 284 | 1024 | 27.7% | 64 | 32 | 32486 / 282 | 1.04 | 12 | 0 (0) | 6 | 492 (569) | 1319 | 1338 |
| 16 | grow | C | after creation | 2 | 16 | 12.5% | 8 | 32 | 0 / 0 | n/a | 0 | 0 (0) | 0 | 0 (0) | 0 | 0 |
| 16 | grow | C | end | 13123 | 32768 | 40.0% | 32 | 32 | 19647 / 13121 | 1.26 | 21 | 1 (16) | 11 | 16354 (18641) | 35449 | 37379 |

### tau's finds (M rows; from the counting runs)

| job | row | found | none | probes | position bytes |
| --- | --- | --- | --- | --- | --- |
| 1 | Hash-M-fixed | 37 | 246 | 410 | 2048 |
| 1 | Hash-M-grow | 37 | 246 | 408 | 2048 |
| 2 | Hash-M-fixed | 21 | 64 | 94 | 2048 |
| 2 | Hash-M-grow | 21 | 64 | 129 | 512 |
| 3 | Hash-M-fixed | 4 | 22 | 26 | 2048 |
| 3 | Hash-M-grow | 4 | 22 | 51 | 128 |
| 4 | Hash-M-fixed | 0 | 8 | 8 | 2048 |
| 4 | Hash-M-grow | 0 | 8 | 8 | 64 |
| 5 | Hash-M-fixed | 118 | 893 | 1269 | 8192 |
| 5 | Hash-M-grow | 118 | 893 | 2072 | 4096 |
| 6 | Hash-M-fixed | 32 | 251 | 298 | 8192 |
| 6 | Hash-M-grow | 32 | 251 | 405 | 2048 |
| 7 | Hash-M-fixed | 19 | 65 | 86 | 8192 |
| 7 | Hash-M-grow | 19 | 65 | 145 | 512 |
| 8 | Hash-M-fixed | 3 | 23 | 26 | 8192 |
| 8 | Hash-M-grow | 3 | 23 | 53 | 128 |
| 9 | Hash-M-fixed | 874 | 2668 | 3917 | 32768 |
| 9 | Hash-M-grow | 874 | 2668 | 5415 | 16384 |
| 10 | Hash-M-fixed | 104 | 902 | 1038 | 32768 |
| 10 | Hash-M-grow | 104 | 902 | 2050 | 4096 |
| 11 | Hash-M-fixed | 37 | 243 | 281 | 32768 |
| 11 | Hash-M-grow | 37 | 243 | 394 | 2048 |
| 12 | Hash-M-fixed | 19 | 69 | 88 | 32768 |
| 12 | Hash-M-grow | 19 | 69 | 122 | 512 |
| 13 | Hash-M-fixed | 5327 | 4795 | 10123 | 262144 |
| 13 | Hash-M-grow | 5327 | 4795 | 10750 | 131072 |
| 14 | Hash-M-fixed | 908 | 2652 | 3560 | 262144 |
| 14 | Hash-M-grow | 908 | 2652 | 5439 | 32768 |
| 15 | Hash-M-fixed | 112 | 903 | 1016 | 262144 |
| 15 | Hash-M-grow | 112 | 903 | 2054 | 8192 |
| 16 | Hash-M-fixed | 34 | 248 | 282 | 262144 |
| 16 | Hash-M-grow | 34 | 248 | 386 | 4096 |

## Also reported (rule-m1 item 13)

### Realized sharing

s: distinct values per matrix of A and B, as drawn; W per store (0 and 1 included) at the end, from the model (the gates checked it).

| job | n | rho | s | distinct A | distinct B | distinct C | W_A | W_B | W_C |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | 16 | 1 | 256 | 256 | 256 | 256 | 286 | 285 | 286 |
| 2 | 16 | 4 | 64 | 64 | 64 | 252 | 88 | 87 | 282 |
| 3 | 16 | 16 | 16 | 16 | 16 | 153 | 28 | 28 | 182 |
| 4 | 16 | 64 | 4 | 4 | 4 | 16 | 9 | 10 | 28 |
| 5 | 32 | 1 | 1024 | 1024 | 1024 | 1024 | 1023 | 1013 | 1045 |
| 6 | 32 | 4 | 256 | 256 | 256 | 1008 | 284 | 285 | 1032 |
| 7 | 32 | 16 | 64 | 64 | 64 | 897 | 92 | 86 | 928 |
| 8 | 32 | 64 | 16 | 16 | 16 | 252 | 28 | 28 | 281 |
| 9 | 64 | 1 | 4096 | 4096 | 4096 | 4095 | 3560 | 3544 | 4014 |
| 10 | 64 | 4 | 1024 | 1024 | 1024 | 4086 | 1014 | 1008 | 3989 |
| 11 | 64 | 16 | 256 | 256 | 256 | 3978 | 280 | 282 | 3895 |
| 12 | 64 | 64 | 64 | 64 | 64 | 2601 | 88 | 90 | 2589 |
| 13 | 128 | 1 | 16384 | 16384 | 16384 | 16329 | 10220 | 10124 | 14719 |
| 14 | 128 | 4 | 4096 | 4096 | 4096 | 16306 | 3541 | 3562 | 14574 |
| 15 | 128 | 16 | 1024 | 1024 | 1024 | 16218 | 1012 | 1017 | 14562 |
| 16 | 128 | 64 | 256 | 256 | 256 | 14447 | 282 | 284 | 13123 |

### Cases per operation (every row takes the same; spec section 10 items 3-4)

- job 1: add (256): int_both 0.008, int_x 0.086, int_y 0.070, den 0.027, general 0.809
- job 2: add (256): int_both 0.023, int_x 0.098, int_y 0.113, den 0.043, general 0.723
- job 3: add (256): int_both 0.012, int_x 0.047, int_y 0.125, den 0.027, general 0.789
- job 4: add (256): general 1.000
- job 5: add (1024): int_both 0.011, int_x 0.090, int_y 0.106, den 0.034, general 0.759
- job 6: add (1024): int_both 0.012, int_x 0.107, int_y 0.086, den 0.030, general 0.765
- job 7: add (1024): int_both 0.003, int_x 0.089, int_y 0.053, den 0.038, general 0.817
- job 8: add (1024): int_both 0.032, int_x 0.201, int_y 0.112, den 0.012, general 0.643
- job 9: add (4096): int_both 0.012, int_x 0.101, int_y 0.100, cross 0.000, den 0.039, general 0.748
- job 10: add (4096): int_both 0.012, int_x 0.103, int_y 0.100, den 0.038, general 0.747
- job 11: add (4096): int_both 0.013, int_x 0.116, int_y 0.108, den 0.031, general 0.733
- job 12: add (4096): int_both 0.008, int_x 0.052, int_y 0.136, den 0.029, general 0.775
- job 13: add (16384): int_both 0.012, int_x 0.096, int_y 0.099, cross 0.000, den 0.033, num 0.000, general 0.760
- job 14: add (16384): zero_x 0.000, int_both 0.012, int_x 0.095, int_y 0.098, den 0.037, general 0.758
- job 15: add (16384): int_both 0.013, int_x 0.096, int_y 0.107, den 0.034, general 0.749
- job 16: add (16384): int_both 0.019, int_x 0.135, int_y 0.119, cross 0.001, den 0.033, general 0.693

### Cross tests per operation (counting runs)

Four cross tests per operation on two cells (spec section 6); V reads the values (cross reads), M compares handles through tau and reads only what a case needs (K-Q3 reads).

| job | operations on cells | cross tests | matched | V cross reads | V K-Q3 reads | M K-Q3 reads |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | 256 | 1024 | 9 | 1024 | 0 | 973 |
| 2 | 256 | 1024 | 17 | 1024 | 0 | 947 |
| 3 | 256 | 1024 | 10 | 1024 | 0 | 967 |
| 4 | 256 | 1024 | 0 | 1024 | 0 | 1024 |
| 5 | 1024 | 4096 | 46 | 4096 | 0 | 3838 |
| 6 | 1024 | 4096 | 43 | 4096 | 0 | 3843 |
| 7 | 1024 | 4096 | 42 | 4096 | 0 | 3906 |
| 8 | 1024 | 4096 | 45 | 4096 | 0 | 3697 |
| 9 | 4096 | 16384 | 210 | 16384 | 0 | 15303 |
| 10 | 4096 | 16384 | 207 | 16384 | 0 | 15300 |
| 11 | 4096 | 16384 | 179 | 16384 | 0 | 15236 |
| 12 | 4096 | 16384 | 150 | 16384 | 0 | 15431 |
| 13 | 16384 | 65536 | 744 | 65536 | 0 | 61411 |
| 14 | 16384 | 65536 | 790 | 65536 | 0 | 61372 |
| 15 | 16384 | 65536 | 783 | 65536 | 0 | 61208 |
| 16 | 16384 | 65536 | 868 | 65536 | 0 | 60198 |

### Bytes per cell (accounted, at the end; tau included)

| job | TC | Hash-V-fixed | Hash-M-fixed | Hash-V-grow | Hash-M-grow | Hash-V-grow-map |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | 16.00 | 30.67 | 33.33 | 30.67 | 33.33 | 30.67 |
| 2 | 16.00 | 30.67 | 33.33 | 14.67 | 15.33 | 14.67 |
| 3 | 16.00 | 30.67 | 33.33 | 8.00 | 8.17 | 8.00 |
| 4 | 16.00 | 30.67 | 33.33 | 5.00 | 5.08 | 5.00 |
| 5 | 16.00 | 30.67 | 33.33 | 20.00 | 21.33 | 20.00 |
| 6 | 16.00 | 30.67 | 33.33 | 14.67 | 15.33 | 14.67 |
| 7 | 16.00 | 30.67 | 33.33 | 8.00 | 8.17 | 8.00 |
| 8 | 16.00 | 30.67 | 33.33 | 5.67 | 5.71 | 5.67 |
| 9 | 16.00 | 30.67 | 33.33 | 17.33 | 18.67 | 17.33 |
| 10 | 16.00 | 30.67 | 33.33 | 9.33 | 9.67 | 9.33 |
| 11 | 16.00 | 30.67 | 33.33 | 8.00 | 8.17 | 8.00 |
| 12 | 16.00 | 30.67 | 33.33 | 7.00 | 7.04 | 7.00 |
| 13 | 16.00 | 34.67 | 40.00 | 21.33 | 24.00 | 21.33 |
| 14 | 16.00 | 34.67 | 40.00 | 13.33 | 14.00 | 13.33 |
| 15 | 16.00 | 34.67 | 40.00 | 11.33 | 11.50 | 11.33 |
| 16 | 16.00 | 34.67 | 40.00 | 11.00 | 11.08 | 11.00 |

The gates checked every counter and store figure against the model (spec section 10 item 2), the TC gate included: MHVF:1 pass, MHMF:1 pass, TC:1 pass, MHVG:1 pass, MHVGM:1 pass, MHMG:1 pass, TC:2 pass, MHVF:2 pass, MHMF:2 pass, MHVG:2 pass, MHMG:2 pass, MHVGM:2 pass, MHVF:3 pass, TC:3 pass, MHMF:3 pass, MHVG:3 pass, MHVGM:3 pass, MHMG:3 pass, TC:4 pass, MHVF:4 pass, MHMF:4 pass, MHMG:4 pass, MHVG:4 pass, MHVGM:4 pass, MHVF:5 pass, TC:5 pass, MHMF:5 pass, MHVG:5 pass, MHVGM:5 pass, MHMG:5 pass, TC:6 pass, MHVF:6 pass, MHVG:6 pass, MHMG:6 pass, MHMF:6 pass, MHVGM:6 pass, TC:7 pass, MHVF:7 pass, MHVG:7 pass, MHMF:7 pass, MHMG:7 pass, MHVGM:7 pass, MHVF:8 pass, TC:8 pass, MHVG:8 pass, MHMF:8 pass, MHMG:8 pass, MHVGM:8 pass, TC:9 pass, MHVF:9 pass, MHVG:9 pass, MHMG:9 pass, MHVGM:9 pass, MHMF:9 pass, TC:10 pass, MHVF:10 pass, MHVG:10 pass, MHMF:10 pass, MHMG:10 pass, MHVGM:10 pass, TC:11 pass, MHVF:11 pass, MHMF:11 pass, MHVG:11 pass, MHMG:11 pass, MHVGM:11 pass, MHVF:12 pass, TC:12 pass, MHVG:12 pass, MHMF:12 pass, MHVGM:12 pass, MHMG:12 pass, TC:13 pass, MHVF:13 pass, MHVG:13 pass, MHMG:13 pass, MHVGM:13 pass, TC:14 pass, MHVF:14 pass, MHMF:13 pass, MHVG:14 pass, MHMF:14 pass, MHVGM:14 pass, TC:15 pass, MHMG:14 pass, MHVF:15 pass, MHVG:15 pass, MHMG:15 pass, MHVGM:15 pass, MHMF:15 pass, TC:16 pass, MHVF:16 pass, MHVG:16 pass, MHMF:16 pass, MHVGM:16 pass, MHMG:16 pass

### The idle and disturbance checks

As round 3 (rule-m1 item 5, carried over in full): processes are told apart by process, not by thread; the fork counter is reported only; host ticks are given as ticks / allowance (none in the pilot). The runner's own ticks are reported in total and per launch; the one sanity limit is more than 1 tick per launch plus 2 over a block.

| job | block | attempt | seconds | disturbed | runner ticks | spawns | runner ticks per launch | forks (reported) | forks less spawns (reported) | new processes under 1 tick (reported) | other limit | credited | host ticks |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | pilot | 1 | 3.719 | no | 3 | 20 | 0.150 | 20 | 0 | none | 9 | none | process_api 0 (6->6 threads), sh 0 (1->1 threads), environment-man 0 (9->9 threads), claude 1 (7->7 threads), bash 0 (1->1 threads) |
| 6 | pilot | 1 | 3.747 | no | 1 | 20 | 0.050 | 21 | 1 | none | 9 | none | process_api 0 (6->6 threads), sh 0 (1->1 threads), environment-man 0 (9->9 threads), claude 0 (7->8 threads), bash 0 (1->1 threads) |
| 13 | pilot | 1 | 3.817 | no | 2 | 20 | 0.100 | 21 | 1 | none | 9 | none | process_api 0 (6->6 threads), sh 0 (1->1 threads), environment-man 0 (9->9 threads), claude 0 (8->8 threads), bash 0 (1->1 threads) |
| 1 | measured | 1 | 19.869 | no | 12 | 90 | 0.133 | 90 | 0 | none | 25 | none | process_api 1/16 (6->6 threads), sh 0/16 (1->1 threads), environment-man 0/16 (9->9 threads), claude 1/16 (8->6 threads), bash 0/16 (1->1 threads) |
| 2 | measured | 1 | 20.531 | no | 12 | 90 | 0.133 | 91 | 1 | none | 26 | none | process_api 0/16 (6->6 threads), sh 0/16 (1->1 threads), environment-man 0/16 (9->9 threads), claude 2/17 (7->7 threads), bash 0/16 (1->1 threads) |
| 3 | measured | 1 | 19.383 | no | 13 | 90 | 0.144 | 92 | 2 | none | 25 | none | process_api 0/16 (6->6 threads), sh 0/16 (1->1 threads), environment-man 1/16 (9->9 threads), claude 2/16 (7->7 threads), bash 0/16 (1->1 threads) |
| 4 | measured | 1 | 25.824 | no | 13 | 90 | 0.144 | 93 | 3 | none | 31 | none | process_api 0/19 (6->6 threads), sh 0/19 (1->1 threads), environment-man 0/19 (9->9 threads), claude 3/19 (7->7 threads), bash 0/19 (1->1 threads) |
| 5 | measured | 1 | 17.923 | no | 14 | 90 | 0.156 | 91 | 1 | none | 23 | none | process_api 1/15 (6->6 threads), sh 0/15 (1->1 threads), environment-man 1/15 (9->9 threads), claude 2/15 (7->8 threads), bash 0/15 (1->1 threads) |
| 6 | measured | 1 | 18.366 | no | 13 | 90 | 0.144 | 90 | 0 | none | 24 | none | process_api 0/15 (6->6 threads), sh 0/15 (1->1 threads), environment-man 0/15 (9->9 threads), claude 2/15 (8->7 threads), bash 0/15 (1->1 threads) |
| 7 | measured | 1 | 19.407 | no | 12 | 90 | 0.133 | 92 | 2 | none | 25 | none | process_api 1/16 (6->6 threads), sh 0/16 (1->1 threads), environment-man 0/16 (9->9 threads), claude 3/16 (6->7 threads), bash 0/16 (1->1 threads) |
| 8 | measured | 1 | 17.585 | no | 13 | 90 | 0.144 | 91 | 1 | none | 23 | none | process_api 0/15 (6->6 threads), sh 0/15 (1->1 threads), environment-man 0/15 (9->9 threads), claude 2/15 (7->8 threads), bash 0/15 (1->1 threads) |
| 9 | measured | 1 | 19.865 | no | 13 | 90 | 0.144 | 90 | 0 | none | 25 | none | process_api 1/16 (6->6 threads), sh 0/16 (1->1 threads), environment-man 1/16 (9->9 threads), claude 2/16 (8->6 threads), bash 0/16 (1->1 threads) |
| 10 | measured | 1 | 18.098 | no | 13 | 90 | 0.144 | 92 | 2 | none | 24 | none | process_api 1/15 (6->6 threads), sh 0/15 (1->1 threads), environment-man 1/15 (9->9 threads), claude 2/15 (6->7 threads), bash 0/15 (1->1 threads) |
| 11 | measured | 1 | 22.644 | no | 13 | 90 | 0.144 | 92 | 2 | none | 28 | none | process_api 0/17 (6->6 threads), sh 0/17 (1->1 threads), environment-man 0/17 (9->9 threads), claude 3/18 (7->8 threads), bash 0/17 (1->1 threads) |
| 12 | measured | 1 | 22.482 | no | 12 | 90 | 0.133 | 92 | 2 | none | 28 | none | process_api 1/17 (6->6 threads), sh 0/17 (1->1 threads), environment-man 1/17 (9->9 threads), claude 3/18 (7->8 threads), bash 0/17 (1->1 threads) |
| 13 | measured | 1 | 18.874 | no | 14 | 90 | 0.156 | 90 | 0 | none | 24 | none | process_api 0/15 (6->6 threads), sh 0/15 (1->1 threads), environment-man 0/15 (9->9 threads), claude 1/16 (8->7 threads), bash 0/15 (1->1 threads) |
| 14 | measured | 1 | 22.848 | no | 15 | 90 | 0.167 | 91 | 1 | none | 28 | none | process_api 0/17 (6->6 threads), sh 0/17 (1->1 threads), environment-man 1/17 (9->9 threads), claude 3/18 (7->7 threads), bash 0/17 (1->1 threads) |
| 15 | measured | 1 | 22.056 | no | 13 | 90 | 0.144 | 93 | 3 | none | 28 | none | process_api 0/17 (6->6 threads), sh 0/17 (1->1 threads), environment-man 0/17 (9->9 threads), claude 8/17 (7->8 threads), bash 0/17 (1->1 threads) |
| 16 | measured | 1 | 21.185 | no | 12 | 90 | 0.133 | 91 | 1 | none | 27 | none | process_api 0/17 (6->6 threads), sh 0/17 (1->1 threads), environment-man 0/17 (9->9 threads), claude 3/17 (8->8 threads), bash 0/17 (1->1 threads) |

The runner's own ticks over the 19 blocks: 213 ticks over 1500 launches, 0.142 per launch; the highest block 0.167 per launch (sanity limit: more than 1 per launch plus 2 per block). Reported only: they fall between reps, not in them.

### The measured process's own CPU time (reported only; enters no verdict)

CPU share: the timed process's CPU time (CLOCK_PROCESS_CPUTIME_ID) over the CLOCK_MONOTONIC time of the same whole rep, for the rep kept in each round.

| job | row | rounds | CPU share, median | CPU share, lowest | CPU ns per run, median | total ns per run, median |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | TC | 12 | 0.9980 | 0.9554 | 10654 | 10614 |
| 1 | Hash-V-fixed | 12 | 0.9989 | 0.9793 | 20756 | 20653 |
| 1 | Hash-M-fixed | 12 | 0.9993 | 0.8718 | 24574 | 24445 |
| 1 | Hash-V-grow | 12 | 0.9995 | 0.9946 | 52724 | 52527 |
| 1 | Hash-M-grow | 12 | 0.9965 | 0.9776 | 55887 | 56025 |
| 1 | Hash-V-grow-map | 12 | 0.9994 | 0.9800 | 38998 | 38790 |
| 1 | Hash-V-fixed-null | 12 | 0.9970 | 0.9780 | 20746 | 20731 |
| 2 | TC | 12 | 0.9997 | 0.9912 | 10289 | 10153 |
| 2 | Hash-V-fixed | 12 | 0.9984 | 0.9551 | 19652 | 19568 |
| 2 | Hash-M-fixed | 12 | 0.9972 | 0.9782 | 21875 | 21821 |
| 2 | Hash-V-grow | 12 | 0.9989 | 0.9842 | 31597 | 31626 |
| 2 | Hash-M-grow | 12 | 0.9963 | 0.9884 | 31503 | 31485 |
| 2 | Hash-V-grow-map | 12 | 0.9994 | 0.9853 | 26463 | 26300 |
| 2 | Hash-V-fixed-null | 12 | 0.9996 | 0.9836 | 19642 | 19498 |
| 3 | TC | 12 | 0.9994 | 0.9621 | 10524 | 10417 |
| 3 | Hash-V-fixed | 12 | 0.9987 | 0.9917 | 19183 | 19062 |
| 3 | Hash-M-fixed | 12 | 0.9998 | 0.9934 | 20566 | 20410 |
| 3 | Hash-V-grow | 12 | 0.9995 | 0.9891 | 24925 | 24854 |
| 3 | Hash-M-grow | 12 | 0.9987 | 0.9852 | 24473 | 24425 |
| 3 | Hash-V-grow-map | 12 | 0.9983 | 0.9301 | 22876 | 22749 |
| 3 | Hash-V-fixed-null | 12 | 0.9982 | 0.9897 | 19301 | 19186 |
| 4 | TC | 12 | 0.9998 | 0.9942 | 12348 | 12200 |
| 4 | Hash-V-fixed | 12 | 0.9993 | 0.9977 | 21313 | 21186 |
| 4 | Hash-M-fixed | 12 | 0.9995 | 0.9851 | 22423 | 22303 |
| 4 | Hash-V-grow | 12 | 0.9994 | 0.9945 | 23562 | 23483 |
| 4 | Hash-M-grow | 12 | 0.9995 | 0.9889 | 23450 | 23415 |
| 4 | Hash-V-grow-map | 12 | 0.9995 | 0.9968 | 23414 | 23323 |
| 4 | Hash-V-fixed-null | 12 | 0.9992 | 0.9886 | 21396 | 21312 |
| 5 | TC | 12 | 0.9994 | 0.9857 | 67495 | 67504 |
| 5 | Hash-V-fixed | 12 | 0.9995 | 0.9909 | 124502 | 124514 |
| 5 | Hash-M-fixed | 12 | 0.9999 | 0.9968 | 141605 | 141449 |
| 5 | Hash-V-grow | 12 | 0.9996 | 0.9783 | 241598 | 241369 |
| 5 | Hash-M-grow | 12 | 0.9985 | 0.9892 | 259380 | 259552 |
| 5 | Hash-V-grow-map | 12 | 0.9993 | 0.9906 | 200907 | 201211 |
| 5 | Hash-V-fixed-null | 12 | 0.9996 | 0.9804 | 124346 | 124243 |
| 6 | TC | 12 | 0.9992 | 0.9528 | 67077 | 67084 |
| 6 | Hash-V-fixed | 12 | 0.9989 | 0.9971 | 123238 | 123072 |
| 6 | Hash-M-fixed | 12 | 0.9997 | 0.9885 | 129651 | 129408 |
| 6 | Hash-V-grow | 12 | 0.9997 | 0.9947 | 215969 | 215782 |
| 6 | Hash-M-grow | 12 | 0.9999 | 0.9940 | 220640 | 220853 |
| 6 | Hash-V-grow-map | 12 | 0.9994 | 0.9525 | 179336 | 179552 |
| 6 | Hash-V-fixed-null | 12 | 0.9996 | 0.9898 | 123305 | 123247 |
| 7 | TC | 12 | 0.9988 | 0.9864 | 67230 | 67384 |
| 7 | Hash-V-fixed | 12 | 0.9992 | 0.9464 | 118117 | 117880 |
| 7 | Hash-M-fixed | 12 | 0.9990 | 0.9834 | 117750 | 117608 |
| 7 | Hash-V-grow | 12 | 0.9993 | 0.9640 | 153163 | 153069 |
| 7 | Hash-M-grow | 12 | 0.9991 | 0.9707 | 152404 | 152354 |
| 7 | Hash-V-grow-map | 12 | 0.9989 | 0.9962 | 145069 | 144876 |
| 7 | Hash-V-fixed-null | 12 | 0.9987 | 0.9939 | 117881 | 117632 |
| 8 | TC | 12 | 0.9996 | 0.9933 | 47553 | 47467 |
| 8 | Hash-V-fixed | 12 | 1.0000 | 0.9945 | 102334 | 102037 |
| 8 | Hash-M-fixed | 12 | 0.9996 | 0.9975 | 99467 | 99268 |
| 8 | Hash-V-grow | 12 | 0.9994 | 0.9931 | 122430 | 122456 |
| 8 | Hash-M-grow | 12 | 0.9995 | 0.9819 | 119041 | 118906 |
| 8 | Hash-V-grow-map | 12 | 0.9997 | 0.9946 | 116475 | 116387 |
| 8 | Hash-V-fixed-null | 12 | 1.0000 | 0.9929 | 101587 | 101330 |
| 9 | TC | 12 | 0.9997 | 0.9952 | 366664 | 366450 |
| 9 | Hash-V-fixed | 12 | 0.9996 | 0.9916 | 592168 | 592024 |
| 9 | Hash-M-fixed | 12 | 0.9998 | 0.9981 | 653976 | 654050 |
| 9 | Hash-V-grow | 12 | 0.9998 | 0.9915 | 956531 | 956576 |
| 9 | Hash-M-grow | 12 | 0.9996 | 0.9986 | 1030012 | 1030096 |
| 9 | Hash-V-grow-map | 12 | 0.9998 | 0.9978 | 831068 | 830897 |
| 9 | Hash-V-fixed-null | 12 | 0.9996 | 0.9948 | 590744 | 590850 |
| 10 | TC | 12 | 1.0000 | 0.9998 | 366322 | 366036 |
| 10 | Hash-V-fixed | 12 | 1.0000 | 0.9967 | 585280 | 585498 |
| 10 | Hash-M-fixed | 12 | 0.9999 | 0.9816 | 601497 | 601218 |
| 10 | Hash-V-grow | 12 | 1.0000 | 0.9990 | 777212 | 776840 |
| 10 | Hash-M-grow | 12 | 1.0000 | 0.9998 | 788102 | 787727 |
| 10 | Hash-V-grow-map | 12 | 1.0000 | 0.9962 | 716601 | 716342 |
| 10 | Hash-V-fixed-null | 12 | 1.0000 | 0.9967 | 585992 | 585707 |
| 11 | TC | 12 | 0.9996 | 0.9913 | 357761 | 358623 |
| 11 | Hash-V-fixed | 12 | 0.9984 | 0.9546 | 562880 | 563182 |
| 11 | Hash-M-fixed | 12 | 0.9982 | 0.9751 | 566251 | 566528 |
| 11 | Hash-V-grow | 12 | 0.9975 | 0.8751 | 718385 | 720773 |
| 11 | Hash-M-grow | 12 | 0.9989 | 0.9813 | 715726 | 716721 |
| 11 | Hash-V-grow-map | 12 | 0.9985 | 0.9902 | 665792 | 666226 |
| 11 | Hash-V-fixed-null | 12 | 0.9997 | 0.9910 | 563079 | 564769 |
| 12 | TC | 12 | 0.9995 | 0.9815 | 356153 | 356917 |
| 12 | Hash-V-fixed | 12 | 0.9996 | 0.9907 | 569624 | 570158 |
| 12 | Hash-M-fixed | 12 | 0.9996 | 0.9877 | 576160 | 576111 |
| 12 | Hash-V-grow | 12 | 0.9990 | 0.9961 | 698907 | 698793 |
| 12 | Hash-M-grow | 12 | 0.9993 | 0.9909 | 684213 | 684970 |
| 12 | Hash-V-grow-map | 12 | 0.9955 | 0.9764 | 651219 | 654657 |
| 12 | Hash-V-fixed-null | 12 | 0.9997 | 0.9841 | 573848 | 573651 |
| 13 | TC | 12 | 0.9996 | 0.9900 | 1566089 | 1566800 |
| 13 | Hash-V-fixed | 12 | 0.9987 | 0.9845 | 2547677 | 2554226 |
| 13 | Hash-M-fixed | 12 | 0.9996 | 0.9848 | 2745818 | 2754250 |
| 13 | Hash-V-grow | 12 | 0.9999 | 0.9960 | 3809098 | 3811090 |
| 13 | Hash-M-grow | 12 | 0.9994 | 0.9931 | 4019412 | 4028744 |
| 13 | Hash-V-grow-map | 12 | 1.0000 | 0.9945 | 3371482 | 3370680 |
| 13 | Hash-V-fixed-null | 12 | 0.9998 | 0.9755 | 2549761 | 2549615 |
| 14 | TC | 12 | 0.9989 | 0.9907 | 1564367 | 1565758 |
| 14 | Hash-V-fixed | 12 | 0.9973 | 0.9471 | 2440904 | 2448844 |
| 14 | Hash-M-fixed | 12 | 0.9982 | 0.9794 | 2520403 | 2523779 |
| 14 | Hash-V-grow | 12 | 0.9994 | 0.9832 | 3088956 | 3093161 |
| 14 | Hash-M-grow | 12 | 0.9996 | 0.9960 | 3166255 | 3167230 |
| 14 | Hash-V-grow-map | 12 | 0.9995 | 0.9795 | 2889488 | 2893581 |
| 14 | Hash-V-fixed-null | 12 | 0.9994 | 0.9878 | 2435159 | 2436004 |
| 15 | TC | 12 | 0.9995 | 0.9766 | 1562630 | 1564351 |
| 15 | Hash-V-fixed | 12 | 0.9991 | 0.9834 | 2394602 | 2396365 |
| 15 | Hash-M-fixed | 12 | 0.9995 | 0.9941 | 2439948 | 2441257 |
| 15 | Hash-V-grow | 12 | 0.9990 | 0.9834 | 2919564 | 2930400 |
| 15 | Hash-M-grow | 12 | 0.9985 | 0.9826 | 2912283 | 2924326 |
| 15 | Hash-V-grow-map | 12 | 0.9984 | 0.9850 | 2782805 | 2788461 |
| 15 | Hash-V-fixed-null | 12 | 0.9995 | 0.9856 | 2400868 | 2405183 |
| 16 | TC | 12 | 0.9976 | 0.9529 | 1519746 | 1522886 |
| 16 | Hash-V-fixed | 12 | 0.9996 | 0.9948 | 2336755 | 2338629 |
| 16 | Hash-M-fixed | 12 | 0.9996 | 0.9731 | 2358046 | 2359818 |
| 16 | Hash-V-grow | 12 | 0.9989 | 0.9625 | 2790880 | 2792820 |
| 16 | Hash-M-grow | 12 | 0.9985 | 0.9520 | 2779794 | 2781730 |
| 16 | Hash-V-grow-map | 12 | 0.9987 | 0.9927 | 2659898 | 2660762 |
| 16 | Hash-V-fixed-null | 12 | 0.9998 | 0.9809 | 2328971 | 2329202 |

Idle checks: 19 (0 busy). Host rates R_h: {'1': 0.26198585276395076, '79': 0.26198585276395076, '85': 0.26198585276395076, '93': 0.26890781098081756, '14116': 0.26198585276395076} (measured: {'1': 0.0, '79': 0.0, '85': 0.0, '93': 0.26890781098081756, '14116': 0.0}; R_h is at least 1 tick over the longest pilot block).

### Round 3, for reference only

Round 3's results (Design 3 on streams of single operations) are in [`../../../10-interning/test/results-r3/results.md`](../../../10-interning/test/results-r3/results.md). They enter no measure here.

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

