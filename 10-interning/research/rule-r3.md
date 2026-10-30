# Design 3, round 3: measurement rule

Oct 29, 2026 · **Fixed: approved by the author, Oct 29, 2026**, after one independent review whose fixes are applied. Each threshold is either **carried over** from round 2's fixed rule ([`rule-r2.md`](rule-r2.md), with its two revisions; approved by the author, Oct 29, 2026), and marked so, or new to this round and marked **approved by the author, Oct 29, 2026**. The harness refuses a measured run unless this file has the approved text (by sha256, pinned in the harness), and while any mark of a proposal awaiting approval remains in it, as in round 2. A change after the first trial is a revision, dated and explained at the top. Round 2's rule stays fixed and unedited. Its run ([`../test/results-r2/`](../test/results-r2/results.md)) stays on record as **"three-structure Q32 (retired)"**.

This rule measures the three rows of [spec v0.3](../v0.3.md):
- **TC**: one structure, unchanged from round 2;
- **Q32-index**: two structures, a list of whole numbers searched top to bottom;
- **Q32-hash**: two structures, a fixed table of 32,768 slots.

Their arithmetic is identical, call for call (spec v0.3 §12.1). They differ only in how whole numbers are stored. Results are reported **per measure, with no overall winner**. The profile is **reported, not judged**.

Three points are the author's (Oct 29, 2026), carried over: the 30-minute hard stop; no background process of the runner's own during timed phases; and the rows (now the author's three, spec v0.3 §0).

## Measurement rule

1. **Rows.** Each job runs four rows:
   - **TC** (spec v0.3 §9);
   - **Q32-index** (§7);
   - **Q32-hash** (§8);
   - **Q32-hash-null**: Q32-hash's binary, run as a separate row for the null control (item 8) (**approved by the author, Oct 29, 2026**: the null copy is of Q32-hash, as round 2's was of its Q32 row, and cheap to run; the alternative is a copy of TC).

   Retired, and not run: round 2's three-structure Q32 and Q32-null, and every row retired by round 2 (spec v0.3 §14).

2. **Builds.** As rule-r2 item 2 (carried over), for the three rows:
   - one compiler and one set of flags (`gcc -std=c11 -O2`);
   - the 08 module and `tc.c` unchanged, their sha256 checked before every run;
   - four builds per row;
   - one counting run and one memory run per row and job, with the per-call counter reads of the call-for-call check;
   - repeats on jobs **3 and 10**, which MUST be identical (carried over);
   - a limit of **10 minutes** per counting or memory run (carried over).

   Q32-hash-null shares Q32-hash's gates, counting and memory runs.

3. **Gates before the measured run.** As rule-r2 item 3 (carried over), for each row: the unit tests of spec v0.3 §12.3, and on every job the gate and the exact checks of spec v0.3 §12.1. Those checks cover:
   - every stored value;
   - $W$ in both Q32 rows;
   - every case and K counter;
   - Q32-index's list, searches and entries examined;
   - Q32-hash's table, stores, probes and longest store;
   - the agreement between the two Q32 rows.

   The counting runs MUST pass the call-for-call check: Q32-index's and Q32-hash's per-call arithmetic each equal to TC's. Gates MAY run two at a time, only before the timed steps.

4. **Timing.** As rule-r2 item 4, every number carried over:
   - **12 interleaved rounds**, the order rotated by one row per round over the four rows;
   - **r**, the smallest power of two with a rep of at least **50 ms**;
   - **3 reps** per invocation, with the first discarded and the minimum of the other two kept, or **1 rep** if the calibration rep lasts at least **1 s**;
   - calibration per job just before round 1, for TC, Q32-index and Q32-hash, with the null row taking Q32-hash's r;
   - pinning to **core 1**, with the runner on core 0;
   - a time cap of **10 s** per rep (over cap is a reported finding);
   - the process's own CPU time, reported only.

   **The parts** (spec v0.3 §11.3):
   - **creation**: `create` and every entering call (`create` timed: **approved by the author, Oct 29, 2026**, spec R3-4). `create`'s own time is also reported separately, per run and as a share of the creation part, with no verdict, so its share is visible, especially on small jobs such as job 3;
   - **operations**: every add, sub, mul and div call;
   - **read-out**.

   A row's time in a round is each part's time per run, and the **total** is their sum.

5. **Nothing of the runner's own runs during a timed phase.** **Carried over in full** from rule-r2 item 5 with both its revisions. This covers:
   - the single-threaded runner;
   - the start of each timed invocation by `fork` and `exec`, with `sched_setaffinity` to core 1 and `dup2` to a file before `exec`, and with no `posix_spawn` or `subprocess`;
   - one blocking `waitpid`;
   - the kernel timer for the hard stop;
   - no writes during an invocation;
   - the operator's duty to start the runner once, as one background command with a timeout of at least **35 minutes**, and to run nothing else until it exits.

   **The disturbance check by process**, after each timed block, is carried over with all its numbers:
   - snapshots by process with start times;
   - ticks including reaped children;
   - the credit for children reaped during the block;
   - the fork counter reported only;
   - other processes limited to $5 + \lceil 1 \cdot L \rceil$ ticks;
   - a new process limited to under 1 tick;
   - host processes held to the allowance $5 + \lceil 2\,R_h\,L \rceil$, with $R_h$ from the pilot and at least 1 tick over the longest pilot block;
   - a stop if any host's measured rate exceeds **4 ticks per second**;
   - a disturbed block re-run once, and a second disturbance stopping the run.

   **The runner's ticks** are reported in total and per launch, with the sanity limit of **more than 1 tick per launch plus 2** per block, a harness bug (carried over). The one addition, untimed: the runner reads `CLOCK_MONOTONIC` just before each `fork` and just after its `waitpid` returns. This is one vDSO read each, with no I/O. The runner keeps the readings in memory, to give the pilot's $P_\text{idx}$ (Run plan step 3).

6. **Idle machine.** Carried over from rule-r2 item 6: a **2-second** sample before each timed block. The machine is idle when no process outside the runner used more than **5%** of one core and core 1 was at most **2%** busy. If it is busy, the runner re-checks every **15 s** and stops after **10 minutes**.

7. **Measures**, per job, for each **pairing**:
   - **Q32-index against TC**;
   - **Q32-hash against TC**;
   - **Q32-hash against Q32-index** (**approved by the author, Oct 29, 2026**: the author's model has two structures, and this pairing compares its two forms directly; without it, the two forms are compared only through TC).

   The measures are:
   - **Creation time.** T_c(reference) / T_c(row), with the reference TC, or Q32-index in the third pairing. T_c is the median over rounds of the creation part per run. Above 1, the row is faster. The 95% interval [lo, hi] comes from **10,000 paired bootstrap resamples** of the rounds, with seed 20261029 (carried over).
   - **Operation time.** The same, on the operations part.
   - **Total time and read-out time.** Reported with ratio and interval, **without a verdict** (the author: creation and operations are the measures; the total is reported too).
   - **Bytes per rational, accounted (B_used):**
     - TC: 16;
     - Q32-index: $4 + 8W/N$;
     - Q32-hash: $4 + 262{,}144/N$, the table counted in full whatever its fill.
   - **Bytes per rational, heap (B_heap):** the growth of `mallinfo2` (`uordblks + hblkhd`) from just before `create`, over $N$, with `mallopt` as in round 2 (carried over). Both memory figures come from one memory run per row and job, after entry and at the end. Verdicts use the end figures.
   - **Gcd work: a check, not a verdict** (carried over). G(row)/G(TC) MUST be exactly 1 for both Q32 rows, by the call-for-call check. It is reported with G per operation. Any other value is a bug.

8. **Null control.** Q32-hash against Q32-hash-null, by item 7, gives a null spread **for each time measure**: max(1 − lo, hi − 1, 0) of the creation ratio, and likewise of the operation ratio. Each time verdict uses its own measure's band, **h = max(5%, that measure's null spread)** (the 5% floor carried over; one band per measure: **approved by the author, Oct 29, 2026**, since round 2 had one time measure). The same h applies to every pairing.

   **Caveat: Q32-index's own noise is not measured.** The null row is a copy of Q32-hash, not of Q32-index. On the large jobs, where its calibration rep lasts at least 1 s, Q32-index runs 1 rep per invocation with no discarded rep (item 4), so its noise may differ from Q32-hash's. The shared band h is used for the pairings with Q32-index anyway, and the report states this caveat beside each of their time verdicts.

9. **Verdicts per job, pairing and measure, with magnitudes** (thresholds carried over):

   | Measure | Row wins | Row loses | Tie | Magnitude reported |
   | --- | --- | --- | --- | --- |
   | Creation time | lo > 1 + h_c | hi < 1 − h_c | otherwise | ratio, [lo, hi], h_c |
   | Operation time | lo > 1 + h_o | hi < 1 − h_o | otherwise | ratio, [lo, hi], h_o |
   | B_used (end) | ratio ≤ 0.90 | ratio ≥ 1.10 | otherwise | ratio after entry and at the end; W, N, W/N, fill |
   | B_heap (end) | ratio ≤ 0.90 | ratio ≥ 1.10 | otherwise | ratio after entry and at the end |

   - The total time and read-out time are given beside the verdicts, with ratio and interval, and get no verdict.
   - A memory ratio of at least **2.0** raises the severe-memory-loss flag (carried over).
   - A row over cap has the verdict **over cap** on both time measures.
   - A verdict is never reported without its magnitude.

10. **The profile: reported, not judged.** As rule-r2 item 10, every number carried over:
    - K = **32**;
    - **3 invocations per row**, at the timing build's r, with 3 reps each and the first discarded;
    - the slowdown limit of **1.5**, with an invocation over it having its phase figures withheld;
    - per-case shares for cases with at least **100** sampled calls.

    The profile runs for the three rows (not the null row), in the order Q32-index, Q32-hash, TC, repeated three times per job. The phases are adapted per row (spec v0.3 §11.4). I is the linear search and append in Q32-index, and the hashing and probing in Q32-hash. M covers list growth only, and `create` is reported as its own batch figure. The per-phase differences of each Q32 row from TC, and of Q32-hash from Q32-index, are labelled **indicative only**.

11. **Strata, and no overall winner.** As rule-r2 item 11 (carried over): MONEY (jobs 1–2), SMALL (3–4) and CHAINS (8–11). For each stratum, pairing and measure, the report gives the counts of wins, ties, losses, over-cap and n/a, the median ratio, and the severe flags. The rule combines no measures and names no better row.

12. **Checks, and bugs after the gates.** As rule-r2 item 12 (carried over). Every stored value of every invocation is checked against `fractions`. After the gates, each of these is a **bug**:
    - a check failure;
    - an overflow;
    - a failed exact check of spec v0.3 §12.1;
    - a non-repeating count;
    - either Q32 row reaching its capacity, which for Q32-hash is a full table;
    - gcd work differing between rows.

    A fix to `q32i.c` re-runs Q32-index. A fix to `q32h.c` re-runs Q32-hash and its null row. A fix to `tc.c` is not expected, since `tc.c` is unchanged from round 2; if one is needed, it re-runs TC.

13. **Also reported:**
    - **Storage, per job, after entry and at the end:**
      - Q32-hash's **fill** ($W/32{,}768$), **average probes per store** and **longest store**;
      - Q32-index's **average entries examined per search**, with the searches split into found and appended.
    - **Speed against fill.** A table and a plot across the 8 jobs, sorted by Q32-hash's fill at the end. It shows each row's creation and operation time per call (ns), and the ratios to TC, beside the fill and the average probes or entries examined. It is presented, not judged, so the curve of speed against fill is visible. The jobs differ in more than fill (their operations, cases and reuse differ too), so the curve is descriptive. A dedicated fill sweep is left to a later round (spec v0.3 §0 item 3).
    - Realized reuse, counts per operation, the known-answer skips with their predictions, memory against reuse, the idle and disturbance checks (with the runner's ticks per launch), and the process's own CPU time: as rule-r2 item 13 (carried over).
    - **Round 2, for reference only:** a link to the three-structure Q32 (retired) results. They enter no round-3 measure.

## Workloads

**Exactly round 2's** (carried over): the same generator (`test/gen.py` via `r2_gen.py`), master seed 20261029, job $n$ seeded 20261029 + $n$. Every workload file is **byte-identical** to round 1's and round 2's, checked by sha256 against round 1's pinned `results-q32/state.json`. The jobs that fit (capacity 32,768) are run:

| Job | Name | Stratum | $W$ at the end | Q32-hash fill | Run |
| --- | --- | --- | --- | --- | --- |
| 1 | MONEY-wide | MONEY | 29,866 | 91.1% | yes |
| 2 | MONEY-catalog | MONEY | 15,403 | 47.0% | yes |
| 3 | SMALL8 | SMALL | 350 | 1.1% | yes |
| 4 | SMALL64 | SMALL | 21,346 | 65.1% | yes |
| 5 | MOD-mixed | MOD | 40,849 | — | **no: over capacity** |
| 6 | MOD-muldiv | MOD | 40,798 | — | **no: over capacity** |
| 7 | NEW-add | NEW | 50,795 | — | **no: over capacity** |
| 8 | CH-money | CHAINS | 27,881 | 85.1% | yes |
| 9 | CH-closed | CHAINS | 28,379 | 86.6% | yes |
| 10 | CH-small | CHAINS | 10,945 | 33.4% | yes |
| 11 | CH-mixed | CHAINS | 12,682 | 38.7% | yes |

Both Q32 rows store the same whole numbers as round 2's Q32, so $W$ is unchanged. The table has no fill limit; its fill is set by the jobs alone.

## Run plan

1. Build every row in its four builds, check the hashes of the 08 module and `tc.c`, and generate or verify the workloads, references and predictions. The predictions include Q32-index's entries examined and Q32-hash's probes.
2. Unit tests of the three rows (test build, and the sanitizer build).
3. **Pilot:** jobs **1, 3 and 10** (carried over), with every row, r calibration and **2 rounds**, then **one profiling run** of each of the three rows on **job 1** (carried over), in its own block. It records:
   - the host rates $R_h$;
   - each row's profiling slowdown;
   - $P_\text{prof}$, the profiling block's wall time;
   - $P_\text{idx}$, the summed wall time of Q32-index's launches in the pilot's round blocks (from the runner's readings around `fork` and `waitpid`, item 5);
   - $P_\text{rest}$, the rest of the pilot's wall time.

   **Stop rule** (**approved by the author, Oct 29, 2026**: one term added to round 2's formula):
   $$\text{projection} = \text{elapsed} + 5\ \text{min} \times \max(1,\ P_\text{rest} / 0.5\ \text{min}) + 42 \times P_\text{idx} + 36 \times P_\text{prof}.$$
   - The 5 and 0.5 minutes (carried over) cover the work outside Q32-index: TC, Q32-hash and its null row cost about what round 2's three rows did.
   - The factor 36 (carried over) is 24 × 1.5, since step 7 makes 72 profile invocations against the pilot's 3.
   - **The factor 42 (approved by the author, Oct 29, 2026)** projects Q32-index's own remaining work. Its pilot makes 7 reps on jobs 1, 3 and 10. In steps 4–7 it makes about 37 timed reps per job plus about 5 run-equivalents of gates, counting and memory runs, so about 6 times as many runs. Its search work over all 8 jobs is 3.4 to 4.6 times that of the pilot's three jobs (the bounds below). That gives 6 × 4.6 = 28, and half again for margin gives 42.
   - **If the projection exceeds 22 minutes (carried over), the run stops** and is reported to the author before anything else runs. It also stops if any host's measured rate exceeds 4 ticks per second, or any row's profiling slowdown exceeds 1.5 (carried over).
4. Gates (item 3), two at a time.
5. Counting and memory runs, two at a time. Then the worker pool is shut down and joined.
6. Per job, in job order: the idle check, then calibration (3 launches) and the 12 rounds (48 launches), then the disturbance check.
7. Per job, in job order: the idle check, then the profile runs (9 launches), then the disturbance check.
8. Analysis and report, untimed, after the last timed block.

**Hard stop at 30 minutes (the author's, carried over),** from step 1 to the end of step 7, re-runs included, enforced by the kernel timer. A stopped run reports no verdict.

### Runtime estimate (non-normative)

**Q32-index's search work is the unknown.** It is exact and deterministic: the model predicts it before any timing. It is bounded here from round 2's counts (`test/results-r2/results.md`, counts per operation and realized reuse). A new whole number scans the whole list, which gives $W(W-1)/2$ entries per run over a job. A found one scans at least 1 entry and at most $W$.

| Job | W | searches | entries examined per run (lower – upper) | avg per search | at 0.4 ns per entry |
| --- | --- | --- | --- | --- | --- |
| 1 | 29,866 | 59,415 | 4.5e8 – 1.3e9 | 7,500 – 22,400 | 0.18 – 0.53 s |
| 2 | 15,403 | 59,390 | 1.2e8 – 8.0e8 | 2,000 – 13,400 | 0.05 – 0.32 s |
| 3 | 350 | 53,699 | 1.1e5 – 1.9e7 | 2 – 350 | < 0.01 s |
| 4 | 21,346 | 59,435 | 2.3e8 – 1.0e9 | 3,800 – 17,500 | 0.09 – 0.42 s |
| 8 | 27,881 | 83,093 | 3.9e8 – 1.9e9 | 4,700 – 23,200 | 0.16 – 0.77 s |
| 9 | 28,379 | 83,993 | 4.0e8 – 2.0e9 | 4,800 – 23,600 | 0.16 – 0.79 s |
| 10 | 10,945 | 35,926 | 6.0e7 – 3.3e8 | 1,700 – 9,300 | 0.02 – 0.13 s |
| 11 | 12,682 | 35,951 | 8.0e7 – 3.8e8 | 2,200 – 10,400 | 0.03 – 0.15 s |
| all | | | 1.7e9 – 7.8e9 | | 0.7 – 3.1 s |

The machine is a 2.7 GHz Xeon with gcc 13, which does not vectorize a loop with an early exit at `-O2`. So the scalar loop compares about one entry per cycle or a little slower, about **0.4 ns per entry**. A pessimistic **1 ns** gives 1.7 – 7.8 s for one run of all eight jobs. For scale, round 2's Q32 took 1.1 – 3.7 ms per run, so Q32-index is expected to be about 50 to 500 times slower than TC on the large jobs.

| Part | At 0.4 ns per entry | At 1 ns per entry |
| --- | --- | --- |
| Q32-index, step 6 (about 37 reps per job; 13 where a rep lasts at least 1 s) | 26 – 115 s | 64 – 200 s |
| Q32-index, step 7 (9 reps per job, slowdown about 1) | 6 – 28 s | 15 – 70 s |
| Q32-index, gates, counting and memory runs (two cores) | 3 – 16 s | 8 – 40 s |
| Everything else (round 2's run took 2.5 min; Q32-hash and its null row add about a third of the rounds) | 3 – 3.5 min | 3 – 3.5 min |
| **Whole run** | **about 4 – 6 min** | **about 5 – 9 min** |
| Pilot projection (elapsed about 1 min, + 5 + 42 × P_idx + 36 × P_prof) | about 9 – 12 min | about 11 – 18 min |

- **Time cap (10 s per rep, carried over):** the longest rep is a single run of job 9, at most 0.8 s at 0.4 ns and 2 s at 1 ns. That is well under the cap, which needs no change.
- **Hard stop (30 min) and projection limit (22 min), carried over:** both hold at both rates. The projection would pass 22 minutes, and stop the run for the author, only if the search ran at about 2 ns per entry or worse. That would be a real surprise worth stopping for.
- No adjustment of the cap or the hard stop is proposed.

*Q32-hash* stores by hashing and probing in a 256 KB table that fits in cache. It is expected to cost about what round 2's Q32 did, or less, since a probe reads the value directly rather than through an index. The measured blocks are longer than round 2's: 51 launches instead of 38, and up to about a minute on jobs 8 and 9. The disturbance limits scale with the block's length, and the runner's sanity limit scales with its launches, both as carried over.

## Limitations, reported with the results

- **Textbook only.** No row is optimized. In particular, Q32-index's search is the plain linear search, and Q32-hash's table is plain open addressing with linear probing.
- **Fill is not controlled.** Q32-hash's fill varies across jobs only as the jobs' $W$ does, together with every other property of the jobs. The speed-against-fill table is descriptive. A sweep at target fills is for a later round (spec v0.3 §0 item 3).
- **Q32-index's cost grows with W.** Its time per store is roughly proportional to the list's length, so its verdicts depend on each job's size, not only on its reuse.
- **Every result is kept;** chains do not update in place. Arithmetic is 64-bit only. The profile perturbs what it measures. Jobs 5–7 are not run. The disturbance check cannot see every process. All of these are as round 2.
- **Round 2's Q32 is retired, not superseded.** Its results measured a three-structure design and stay on record under that label.

## Harness

Round 2's harness (`r2_*.py`, `bench_r2.c`, `api_r2.h`) is the starting point. New `r3_*` files sit beside it, and round 2's files stay unchanged. The changes:
- the model predicts both Q32 rows' storage exactly: Q32-index's list order and entries examined, and Q32-hash's table, probes and longest store;
- the driver drives the three rows and times `create` in the creation part;
- the runner runs four rows and records $P_\text{idx}$;
- the analysis reports items 7–13, with three pairings and two time measures.

**The official data.** Round 3 writes only to **`test/results-r3/`**. The harness refuses to write in every earlier results folder:
- `test/results/`;
- `test/results-q64-b3b88b6/`;
- `test/results-q32/`;
- `test/results-r2-stopped-193ae57/`;
- `test/results-r2-stopped-173fad1/`;
- `test/results-r2/` (round 2's complete run, the three-structure Q32 (retired)).

It checks the sha256 of every file in them before and after the run. Builder decisions go in `test/DECISIONS.md` under a round-3 heading, pending the author's review.
