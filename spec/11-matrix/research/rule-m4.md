> **Status: draft, paused, not reviewed.** Paused with the matrix work while overflow is solved at the level of integers (spec 12).

# Design 3 on matrices, round m4: measurement rule

Oct 30, 2026 · **Draft for the author's approval.** It records the author's decisions of Oct 30, 2026 for this rule (items A1–A8 below) and the coordinator's calls that carry them out (M1–M14). M1, M4 and M8 are **approved by the author, Oct 30, 2026**; the others are marked **coordinator's call** where they appear and are pending the author's confirmation. **Revised the same day by the author's answers:** the null row is Packed's binary (M1); memory keeps rule-m3's ±10% parity bands and time keeps h's 5% floor (M4); "no division test" means no division job set, and the correctness tests stay, the large trial-division unit tests included (M8); Packed-coprime's trial-division bound is 3 (v0.6 being updated). Points still undecided are listed at the end (Open points). The harness refuses a measured run unless this file has the approved text (by sha256, pinned in the harness), and while any mark of a proposal awaiting approval remains in it; a "coordinator's call" mark is such a mark (carried over in form). Nothing is built, run or measured.

This is a new rule for specs [v0.4](../v0.4.md) (Control and Packed), [v0.5](../v0.5.md) (Packed-23) and [v0.6](../v0.6.md) (Packed-coprime, still being finished; Open point 2). [`rule-m1.md`](rule-m1.md), [`rule-m2.md`](rule-m2.md) and [`rule-m3.md`](rule-m3.md) stay fixed and unedited. Their runs' results stand as measured and enter no measure here. Each threshold and rule is marked in one of three ways:
- **carried over** from rule-m3, and through it from rule-m2 and rule-m1 (approved by the author, Oct 29–30, 2026);
- **the author's decision, Oct 30, 2026** (A1–A8);
- **coordinator's call** (M2, M3, M5–M7, M9–M14), pending; M1, M4 and M8 are approved by the author, Oct 30, 2026.

A change after the first trial is a revision, dated and explained at the top.

**Language.** The build moves to Nim (`spec/nim/`, in progress; v0.4 §17, v0.5 §18 and v0.6 §19 "Build language"). The rule names no language as a requirement. Where it names a file, flag or routine of the current build (`tc3.c`, `bench_m4.c`, `-d:danger`), that is a place to check, not a requirement of the rule.

## The author's decisions for this rule (Oct 30, 2026)

| # | Decision | Where |
| --- | --- | --- |
| A1 | **Grid:** the same as rule-m3: $n \in \{16, 32, 64\}$, four sharing levels, 12 jobs per set (9 for sop) | item 1, Workloads |
| A2 | **Models:** Control, Packed, Packed-23 and Packed-coprime, all four | The rows |
| A3 | **Comparisons:** every model against Control; Packed-23 against Packed; Packed-coprime against Packed; Packed-23 against Packed-coprime | item 7 |
| A4 | **Measures:** rule-m3's eight (build time per matrix; add, mul, sop and pos times; stable bytes per stored matrix; peak bytes during add and during mul), reported as **× Control** (1.00 = Control, lower is better), **median [range]** | items 7, 11 |
| A5 | **Verdicts:** a difference counts only if it is larger than the measured noise; within noise is **parity**. Best estimates are reported either way | items 8, 9 |
| A6 | **Overflow:** each job's overflow counts ("new" and "passed on", marked cells) are reported per model and operation, as a finding; marked cells count in the times (v0.4 §17.10) | item 14 |
| A7 | **Timing method:** unchanged from rule-m3: idle check, disturbance check, single-threaded runner, pinned core, hard stop, pilot projection limit, per-rep cap with over-cap reporting | items 4–6, Run plan |
| A8 | **No division test this round:** no division job set in the timed round; the correctness tests stay, the large trial-division unit tests included (the author's answer, Oct 30, 2026) | item 3, Run plan (M8) |

## The rows

The four models of v0.4 §17.0, v0.5 §18.0 and v0.6 §19.0 (the author's decision A2), and one null row. No model pre-stores 0 or 1 (v0.2 §15.2).

| Model | Stored, per matrix | Temporary, per operation | Local, per function | Spec |
| --- | --- | --- | --- | --- |
| **Control** | none beyond its cells (the pairs) | none | none | v0.4 §17.0, §17.13 |
| **Packed** | (l) the packed list | (h) the index table, from the build's `create` to its pack | mul's row buffer | v0.4 §17.2–17.8 |
| **Packed-23** | (l), with five-field entries after the pack | (h) | mul's row buffer, with $A$'s denominator factors | v0.5 §18.3–18.7 |
| **Packed-coprime** | (l), with exponent entries and the base's pieces after the pass | (h) | mul's row buffer, with $A$'s denominator exponents | v0.6 §19.3–19.8 |
| **Packed-null** | Packed's binary, run as its own row | | | item 8 |

- All four use the same 64-bit core, the new gcd (v0.4 §17.7), the dot sum from the first product (v0.4 §17.6) and the overflow mark (v0.4 §17.10), and read the same inputs.
- **Control and Packed are identical call for call** (v0.4 §17.12), gcd counts included.
- **The two factored models** take Control's cases and core arithmetic call for call, and obtain their gcd values by S1 and S2 (v0.5 §18.5, v0.6 §19.6). Their gcd counts are reported, not compared (v0.5 C6, v0.6 C8).
- **One build language for all four** (coordinator's call M7): every model of a timed comparison is built from the same language, compiler, version and flags. No timed pairing mixes a C model with a Nim one. *Why:* the Nim pilot ([`../nim-pilot/REPORT.md`](../nim-pilot/REPORT.md) §5) found code-shape effects of a few percent between the languages (branch shape, inlining), the size of the 5% band.
- Results are reported **per measure, with no overall winner** (carried over).

**Four job sets, four runs** (carried over): **add**, **mul**, **sop** ($A \cdot B + C \cdot D$), **pos** ($(A + B) \cdot (C + D)$). Each run has its own steps 0–7, pilot, hard stop, results folder and report. Nothing measured in one run enters a measure of another.

**Run order** (carried over): add, mul, sop, pos, **back to back**. A run that stops does not stop the next one, and its report says so.

## Measurement rule

1. **Rows.** Each job runs **five rows**, in this order for the rounds: **Control, Packed, Packed-23, Packed-coprime, Packed-null** (M1, approved by the author, Oct 30, 2026). The null row is Packed's timing binary, and stays last, as in m1–m3.
   - *Why Packed:* it is the plainest packed model, the base that both factored models extend, and the reference of two pairings, as m3's null row was Hash-V-pack's.
   - **The grid** (the author's decision A1; Workloads): $n \le 64$ for every row and set, and sop without $\rho = 16$. Every row runs every job of its set. There is no "not run" cell.
   - **The 10 s cap applies as carried over**, with over-cap reporting. A row whose calibration rep passes 10 s runs no rounds on that job. Its time verdicts are **over cap**, with the calibration rep's time as the magnitude. Its memory verdicts stand, from the memory run. Expected over cap: none (Runtime estimate; the slowest row is expected near 0.1 s per run).

2. **Builds.** As rule-m3 item 2, in a language-neutral form (coordinator's call M7):
   - **one build setting for all four models**, recorded and pinned by the harness: the compiler and its version (for Nim, the release or nightly with its git hash and tarball sha256, as the Nim pilot records them), the flags, and each model source's sha256. *In the current build:* `--app:staticlib --noMain -d:danger --mm:arc -d:useMalloc --panics:on`, with the C backend at C's `-O2` (Nim pilot §1);
   - **the driver** (now `test/bench_m4.c` and the shared objects) is the same code for every model, so its clock reads, accounting and heap wrapper are identical;
   - **pins checked before every run:** the new gcd's pin (v0.4 §17.7), 08's files and pin, `tc3.c` (the TC gate's reference, not ported), m1's to m3's sources and results, and step 0's files;
   - **Control's new source** differs from the pinned Control (`tcm.c`) only by the summation and the marking (v0.4 §17.13), checked by its recorded diff, in whatever language it is written;
   - **four builds per model:** timing, counting, test and memory, as the current build names them (`spec/nim/src/core.nim`), plus the test build under the sanitizers for step 2;
   - one counting and one memory run per row and job, with a limit of **10 minutes** each (carried over);
   - repeats on jobs **1 and 6** of each set identical (carried over);
   - the same binaries for all four runs (carried over).

3. **Gates before the measured run.** As rule-m3 item 3 (carried over), for every row and job of the set:
   - the unit tests of v0.4 §17.12, v0.5 §18.13 and v0.6 §19.13, for their models;
   - the exact checks of those sections, every step of an expression included, with marked cells as v0.4 §17.10 item 6 says;
   - **the call-for-call check:** Control against Packed in full, gcd counts included (v0.4 §17.12); the factored models in the adapted form of v0.5 §18.13 and v0.6 §19.13 (the same case and core arithmetic as Control, the same read counters as Packed, every obtained gcd value equal to the new gcd's);
   - **the TC gate**, in v0.4 §17.10's form: in full on a job with no marked cell, and up to the first overflow on a job with marked cells, as the simulator decides before any run.

   Gates MAY run two at a time, only before the timed steps (carried over). **No division test** (the author's decision A8; M8, approved by the author, Oct 30, 2026): no job set times a division (v0.1 §1 scope). The correctness tests all stay, **the large trial-division unit tests included**, as rule-m3's step 2 ran them. Packed-coprime's trial division is bounded at **3** (the author, Oct 30, 2026; v0.6 item 11a is being updated from 7), and is covered by v0.6 §19.13's unit tests.

4. **Timing.** As rule-m3 item 4, every number carried over (the author's decision A7):
   - 12 interleaved rounds, rotated by one row per round over the five rows;
   - r, the smallest power of two with a rep of at least 50 ms;
   - 3 reps per invocation, the first discarded and the minimum of the other two kept, or 1 if the calibration rep lasts at least 1 s;
   - calibration per job for the four models, the null row taking Packed's r;
   - core 1, the runner on core 0;
   - a cap of **10 s** per rep;
   - the process's CPU time reported only.

   **The parts** are rule-m3's, with what each now covers:
   - **one build per input matrix**: its `create`, its `enter` calls, its pack and, in the factored models, the pass after the pack (v0.5 §18.4, v0.6 §19.4);
   - **the operation**: every step, with its result's `create`, Packed-coprime's reconciliation (v0.6 §19.5), the step's arithmetic with its marked cells (v0.4 §17.10 item 6), the result's pack and pass, and the freeing of intermediates;
   - **the read-out** of the final result.

   **The driver's scan for marked cells comes after the clock** (v0.4 §17.10 item 5) and is in no timed part. So a result's factoring is operation time, and an input's is build time, as in m3.

5. **Nothing of the runner's own runs during a timed phase.** Carried over in full from rule-m3 item 5, every number included:
   - the single-threaded runner, `fork`/`exec` with `sched_setaffinity` to core 1 and `dup2` to a file, one blocking `waitpid`, the kernel timer for the hard stop, no writes during an invocation;
   - other processes at most $5 + \lceil L \rceil$ ticks; a new process under 1 tick;
   - hosts at most $5 + \lceil 2 R_h L \rceil$, with $R_h$ from the run's own pilot;
   - a stop above 4 ticks per second for any host;
   - one re-run of a disturbed block, a second disturbance stopping the run;
   - the runner's ticks reported, with the sanity limit of more than 1 tick per launch plus 2 per block;
   - one background command per run, with a timeout of at least 45 minutes.

6. **Idle machine.** Carried over: a 2-second sample; idle when no process outside the runner used more than 5% of one core and core 1 was at most 2% busy; re-check every 15 s; stop after 10 minutes.

7. **Measures, per job, for each pairing, in each run.**
   - **The eight headline measures** (the author's decision A4; v0.4 §17.11), with their run assignment carried over from rule-m2 item 7:

     | Run | Speed | Memory |
     | --- | --- | --- |
     | add | **1** build time per input matrix; **2** add time | **6** stable bytes per stored matrix; **7** peak bytes during add |
     | mul | **1**; **3** mul time | **6**; **8** peak bytes during mul |
     | sop | **1**; **4** sop time | **6** (sop's peak: reported, no verdict) |
     | pos | **1**; **5** pos time | **6** (pos's peak: reported, no verdict) |

   - **Pairings, with verdicts on every headline measure** (the author's decision A3; verdicts on all six, coordinator's call M3):
     - **against Control:** Packed, Packed-23, Packed-coprime;
     - **Packed-23 against Packed**;
     - **Packed-coprime against Packed**;
     - **Packed-23 against Packed-coprime.**

     In each pairing the first-named model is the **row** and the second the **reference**. Packed against Packed-null is the null pairing (item 8), with no verdict (coordinator's call M6).
   - **The ratio, as reported** (the author's decision A4; its form, coordinator's call M2): **row over reference, lower is better**, 1.00 = the reference. In the pairings against Control that is × Control.
     - **Time (1–5):** $R = T(\text{row}) / T(\text{reference})$. $T$ is the median over rounds of the operation per run (2–5), or the median of the pooled per-matrix samples (1). The 95% interval $[\text{lo}, \text{hi}]$ comes from **10,000 paired bootstrap resamples of the rounds**, seed 20261029, with measure 1's pooling (a resampled round brings all its input matrices' samples), carried over.
     - **Memory (6–8):** row over reference on the accounted bytes per cell, with the heap figures beside them, carried over: measure 6, the inputs' stable bytes summed over their cells summed; measures 7–8, the accounted peak during the operation over the operation's cells. The factored models' grows and shrinks, and Packed-coprime's reconciliation rewrites, are accounting events (v0.5 §18.4, v0.6 §19.11).
     - *The change from m3:* rule-m3 stored time ratios as $T(\text{reference}) / T(\text{row})$, higher better, and its summary inverted them ([`../test/results-m3-summary.md`](../test/results-m3-summary.md)). Here every ratio is reported row over reference. The verdicts are unchanged (item 9).
   - **Gcd work** (coordinator's call M9).
     - **Packed and Packed-null: a check.** G(row)/G(Control) MUST be exactly 1, call for call (carried over; v0.4 §17.7).
     - **Packed-23 and Packed-coprime: reported, not checked** (v0.5 C6, v0.6 C8). G(row)/G(Control) per part, beside their factor work by kind. What is checked instead is item 3's adapted call-for-call check.
   - **Reported, no verdict:** total and read-out time; per-matrix build times; sop's and pos's peaks; v0.4 §17.11's list (the whole job's bytes, each input's build peak, $W$, $T_\text{fix}$, fill, widths, (l)'s bytes, fields narrowed, the row buffer's bytes); and the counters of item 13.

8. **Null control: the measured noise** (the author's decision A5; its form carried over). Packed against Packed-null gives the null spread per time measure, $\max(1 - \text{lo}, \text{hi} - 1, 0)$, computed on $T(\text{null}) / T(\text{Packed})$ as rule-m3 computes it. The band is **h = max(5%, that spread)** for every pairing of the run (carried over; the 5% floor M4, approved by the author, Oct 30, 2026). There are two bands per run, one for measure 1 and one for the run's operation measure (carried over).
   - **Caveat** (carried over): the other rows' own noise is not measured. Verdicts on jobs where a row runs 1 rep per launch are marked "(c)". None is expected (Runtime estimate).

9. **Verdicts and magnitudes.** Carried over, with "tie" named **parity** (the author's decision A5):
   - **time (1–5):** the row **wins** when its whole interval lies below $1/(1 + h)$, that is $\text{hi} < 1/(1 + h)$; it **loses** when $\text{lo} > 1/(1 - h)$; otherwise **parity**. These are rule-m3's bounds (win when lo > 1 + h, lose when hi < 1 − h on $T(\text{reference}) / T(\text{row})$), written for the reported ratio, which is their reciprocal. So every verdict is the one rule-m3's form would give (coordinator's call M2);
   - **memory (6–8):** wins at a ratio ≤ **0.90**, loses at ≥ **1.10**, otherwise parity (carried over; M4, approved by the author, Oct 30, 2026);
   - the severe-memory-loss flag at a ratio of **2.0** or more (carried over);
   - "over cap" on both time measures (carried over);
   - **every verdict, parity included, is reported with its magnitude:** the ratio, $[\text{lo}, \text{hi}]$ and h for time; the ratio after creation and at the end, accounted and heap, for memory (the author's decision A5: best estimates either way; carried over in form).

10. **No profile** (carried over).

11. **Strata, summary, and no overall winner.** Carried over, per run, for the headline measures: NONE, LOW, MID, HIGH ($\rho$ = 1, 4, 16, 64), with counts of verdicts (wins, parity, losses, over cap), median ratios, severe flags, and the size × sharing grid of ratios, presented, not judged. sop has no MID stratum (Workloads). No measures and no runs are combined.
    - **The summary table** (the author's decision A4; its form, coordinator's call M5): per measure and model, the ratio × Control as **median [min–max] across the jobs** that carry it, in the form of `results-m3-summary.md`. Measures 1 and 6 are pooled across the four runs (45 jobs), as there, and also given per run. A second table gives the three inter-model pairings in the same form, × their reference. Beside each cell: its counts of wins, parity and losses.

12. **Checks, and bugs after the gates.** As rule-m3 item 12 (carried over), every step included. After the gates, each of these is a **bug** (v0.4 §17.10, §17.12; v0.5 §18.13; v0.6 §19.10, §19.13):
    - an unmarked cell that differs from the exact reference or from Control's;
    - a set of marked cells that differs from the simulator's, or between models; counts "new" and "passed on" that differ from the simulator's, or between models;
    - a case, core-arithmetic or read counter that differs from item 3's check; for Packed, any gcd count that differs from Control's;
    - an obtained gcd (S1 or S2) that differs from the new gcd's, in the test build;
    - a list, (h) at the pack, $W$ or found/new per store that differs from the simulator's; a factored model's list, after its pass, that differs from the simulator's in any field;
    - a factored entry that does not multiply back to its whole number; a temporary's carried factors that do not multiply back to its denominator;
    - a whole number 0 factored or absorbed;
    - (h) alive during the pass after the pack, or any temporary beyond (h);
    - a Packed-coprime base that is not pairwise coprime, or has more than $K = 4$ pieces; **"base full"** anywhere (it cannot occur under the draw, v0.6 §19.10, since every piece is 7-smooth and pieces are pairwise coprime; coordinator's call M14: found at the gates, the run stops before step 6 and reports it; after the gates, the job is reported as failed, with no verdict);
    - an audit failure of v0.4 §17.12, v0.5 §18.13 or v0.6 §19.13.

    A fix to a model's source re-runs that model and, for Packed, the null row (carried over in form). **An overflow is not a bug:** it is a finding (v0.4 §17.10).

13. **Also reported.** As rule-m3 item 13, with the counters of each model's spec:
    - Packed: v0.4 §17.11's figures and counters;
    - Packed-23: v0.5 §18.13's (S1 calls and S2 hits, factor work by kind and case, the new gcd's calls and work on unfactored parts, entries factored, cells walked, the list's bytes before and after the grow);
    - Packed-coprime: v0.6 §19.13's (S1 calls and their `divide_out` steps, S2 hits, R1 matches, factor work and folds, pass 1's counts, reconciliation's counts, $\lvert B \rvert$ and its pieces per matrix, $E(w)$).

    The m3 runs are linked for reference only.

14. **Overflow, as a finding** (the author's decision A6; v0.4 §17.10 and §17.11). The report gives, **per job, model and operation**, and in sop and pos **per step**:
    - the counts **"new"** and **"passed on"**, from the counting build;
    - **the marked cells** of the result: their count and share of its cells, from the driver's scan after the clock, in every build;
    - the **"mark tests"** counter.

    By v0.4 §17.10 item 6 these are equal in every model; a difference is a bug (item 12), so the table shows each model's figures and a check column. A per-set summary gives the share of cells marked by size, beside v0.4 §17.10's simulated lower bounds (coordinator's call M11).
    - **Marked cells count in the times** (v0.4 §17.10 item 6): a marked cell's time is its work up to the stop, plus the store of the mark, the same rule in every model. They count in the peaks too.
    - **A job with marked cells keeps its verdicts**, with its counts reported beside them (v0.4 §17.10 item 6).

## Workloads

- **Every set:** v0.4 §17.9's draw, for every model: numerators and denominators as products of the primes 2, 3, 5 and 7, each dividing with probability $1/p$ and repeating with probability $1/p$, at most 32767, a random sign on the numerator, reduced; pools of $s = n^2 / \rho$ distinct values per matrix; the order of random calls as written there.
- **Seeds** (carried over; v0.4 §17.9): job $j$ seeded 20261029 + $j$ (add), + 100 + $j$ (mul), + 200 + $j$ (sop), + 300 + $j$ (pos).
- **The generator** (coordinator's call M13): a new `test/m4_gen.py`, writing v0.4 §17.9's draw into the m3 workload format. Python is pinned to CPython 3.13.17 (v0.4 §17.9); step 0 records the version and refuses another. **Step 0's files no longer equal m1's or m2's** by sha256 (v0.4 §17.9). In their place, the harness records each file's sha256 at step 0 of the add run, and every later step 1 and run checks them.
- **The grid** (the author's decision A1, as rule-m3):
  - **add, mul, pos:** $n \in \{16, 32, 64\}$ by $\rho \in \{1, 4, 16, 64\}$, **jobs 1–12**, numbered as rule-m1;
  - **sop:** the same without $\rho = 16$, **jobs 1, 2, 4, 5, 6, 8, 9, 10, 12** (9 jobs);
  - every job keeps its rule-m1 number and seed. Under the new draw it is not the same workload as in m1–m3.
- **Facts that follow** (v0.4 §17.9, not decisions): no input numerator is 0; every value is 7-smooth, and there are only 497 such values up to 32767, so $W \le \min(2n^2, 497)$ for every input (v0.5 §18.8); a pool of 4,096 values takes about 94,000–96,000 drawn rationals.
- **Expected overflow** (v0.4 §17.10, simulated lower bounds on unpooled samples, non-normative): none in add; mul at most about 0.05% of cells at $n = 64$; sop 0–0.2% at $n = 32$ and 0.07–0.39% at $n = 64$; pos 0–0.29% at $n = 32$ and 0.54–0.93% at $n = 64$.

## Run plan (once per run)

Steps 0–7 as rule-m3's, per run. Results go to **`test/results-m4-add/`**, **`-mul/`**, **`-sop/`** and **`-pos/`** (coordinator's call M10).

- **Step 0, outside the clock** (carried over in form): the workloads, the exact references (`fractions`), and the simulator's predictions for all four models: every table and list, every marked cell and count (v0.4 §17.10, step for step with 08's checks), Packed-23's entries and factor work (v0.5 §18.13), and Packed-coprime's bases, entries, reconciliations and factor work (v0.6 §19.13). It also decides, per job, whether it has marked cells (the TC gate, item 3).
- **Step 1:** build every model in its four builds and the sanitizer build, check every pin of item 2 and step 0's files.
- **Step 2:** the unit tests (test build, and under the sanitizers), the differential tests and the allocation checks of every model, **with the large trial-division unit tests** (item 3, M8).
- **Step 3, pilot** (carried over from rule-m3, over the five rows: coordinator's call M12): jobs **1, 6 and 10**, one per size, every row, r calibration and 2 rounds. **Stop rule, carried over:**
  $$\text{projection} = \text{elapsed} + 1.2 \Big[ \sum_{c} N_c \Big( O_c + \sum_{\text{rows}} (K_{\text{row},c} + 6\, P_{\text{row},c}) \Big) + \tfrac{1}{2} \sum_{c} N_c \sum_{\text{rows}} 4\, t_{\text{row},c} \Big],$$

  with every symbol as rule-m3 defines it: $c$ the three sizes with pilot jobs 1, 6 and 10; $N_c$ = 4 per size in add, mul and pos, and 3 in sop; $K_{\text{row},c}$ the row's calibration launch; $P_{\text{row},c}$ the sum of its two pilot round launches, 0 if over cap; $O_c$ the block's checks; $t_{\text{row},c}$ the row's time per run; **elapsed** steps 1–3 as they ran. The rows are the five of item 1.

  **If the projection exceeds 30 minutes, the run stops** (carried over). It also stops on a host above 4 ticks per second.
- **Step 4:** gates, two at a time. **Step 5:** counting and memory runs, two at a time.
- **Step 6:** per job, in job order: the idle check, calibration (4 launches), 12 rounds (60 launches, fewer when a row is over cap), and the disturbance check (carried over).
- **Step 7:** analysis and report, untimed.
- **Hard stop at 40 minutes, per run** (carried over), from step 1 to the end of step 6. A stopped run reports no verdict.

### Runtime estimate (non-normative)

**No model of this round has been timed, on any draw.** The estimate scales m3's measured C times by instruction counts and by the coordinator's judgement for the factored models. The pilot's projection is the guard.

**Basis, with the range used for the worst case.**
- **Control:** m3's TC per run (`results-m3-*/rounds.csv`, the median over each size's jobs), times the new gcd's Ir ratio (v0.4 §17.11: 0.80 build, 0.84 add, 0.63–0.64 mul, sop, pos), times a **draw factor of 0.9 (0.7–1.3)**. *Why below 1:* a rough simulation of mul's dot sums (unpooled, 40 cells per size) found the gcd operands about 27% ($n = 16$) to 7% ($n = 64$) shorter in bits under the new draw than under m3's, because 7-smooth denominators share factors.
- **Nim against C:** 1.0 (0.97–1.05): the Nim pilot's Control is at 0.97–1.00 of C's Ir.
- **Packed over Control:** 2.2 in add, 1.13 in mul, sop and pos (v0.4 §17.11's Ir on m3's draw; m3's Hash-V-pack timed 2.20 and 1.12–1.13).
- **Packed-23 over Packed:** 1.0 (0.8–1.5) in mul, sop and pos; 1.3 (1.0–2.0) in add, where the result's pass weighs more against the cheap operation. *Why near 1:* S1 replaces each gcd with factored arguments by a few remainders by 2 and 3 and, unless S2 applies, a gcd on a small unfactored part (v0.5 §18.5).
- **Packed-coprime over Packed:** 1.3 (1.0–2.0) in mul, sop and pos; 2.0 (1.2–5.0) in add. *Why:* with the trial-division bound at 3, the pieces 2 and 3 are prime and S2 can apply, but pieces holding 5 or 7 (5, 7, 25, 35, …) stay composite, so S1's last gcd runs whenever one of them remains, and splits can occur in pass 1 and reconciliation; `divide_out` runs over up to 4 pieces held as data, and each result's pass makes up to about 8 gcds per denominator entry (v0.6 §19.4, §19.6).
- **Launches**, fitted to m3's blocks: a launch is its reps plus about 12 ms; a block is 64 launches, a 2 s idle check and the disturbance check. This reproduces m3's blocks within a few percent (for example mul job 10: 73 s modelled, 72.6 s measured).
- **Steps 1–2** (m3: 25 s and 42 s in C, the latter with 13–40 s of large trial divisions, kept here at m3's cost): about 2.5 min (1.5–5) for step 1, for Nim compiles of four models in five builds; about 1.3 min (0.7–2.7) for step 2.
- **Steps 4–5:** about 0.5 min (0.5–1.5).

**Time per run of the slowest row, central:** about 1.2 ms in add, 26 ms in mul, 52 ms in sop and 30 ms in pos, all at $n = 64$ (Packed-coprime). At the worst basis, about 0.12 s (sop, $n = 64$); in the stress case below, about 0.6 s. Every rep is far under the 10 s cap and the 1 s one-rep threshold, so every launch makes 3 reps, and no "(c)" mark is expected.

**Every block is near its floor.** Each rep is 50–100 ms by the power-of-two r, so a launch is 0.16–0.31 s, and a block 13–23 s, whatever the models cost, as long as no row passes about 0.1 s per run.

**Steps 1–6 on the clock, and the pilot's projection, minutes:**

| Run | Jobs | Step 6, central | Step 6, worst | Run, central | Run, worst | Projection, central | Projection, worst | Projection margin on 30, worst |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| add | 12 | 3.4 | 3.6 | 8.0 | 13.2 | 8.8 | 14.2 | 2.1 |
| mul | 12 | 3.5 | 3.9 | 8.1 | 13.5 | 8.9 | 14.6 | 2.1 |
| sop | 9 | 2.6 | 3.7 | 7.2 | 13.3 | 7.8 | 14.3 | 2.1 |
| pos | 12 | 3.8 | 4.3 | 8.4 | 13.9 | 9.3 | 15.1 | 2.0 |

- **Worst** is the largest over every combination of the ranges above, and a stress case with Packed-23 at 3 times and Packed-coprime at 10 times Packed with the draw factor at 1.3. As in m3, it is not always at the slowest corner, because r rounds each rep to 50–100 ms.
- **The projection** is the elapsed steps 1–3 plus 1.2 times steps 4–6, so it sits above the run.
- **Step 0, outside the clock:** add under 1 minute; mul 1–3, sop 2–6, pos 1–4 (m3 measured 3 s, 54 s, 185 s and 66 s, with Pack-coprime's refinement simulation, now gone, and without the step-by-step overflow simulation and the two factored simulators, now added).
- **The whole round**, four runs back to back with step 0: about 41 minutes central, about 68 at the worst basis. m3 took about 48 (43 on the clock).

**Margins.**
- **Hard stop (40 min):** about 2.9 at the worst basis (40 over 13.9), about 4.8 central.
- **Projection limit (30 min):** at least 2.0 at the worst basis, at least 3.2 central.
- **A re-run** of a disturbed $n = 64$ block adds about 0.4 minutes.

**No threshold of rule-m3 changes.** The rows, the draw and the reporting direction change; the grid, the pilot jobs and the projection's form do not.

## Limitations, reported with the results

Carried over from rule-m3, without its Pack-primes and Pack-coprime items. Also:
- **No $n = 128$ in any set, and no $\rho = 16$ in sop** (the author's decision A1). The step from 64 to 128, where m2 saw the handle width change, is not measured. m3 dropped them for the factored rows' cost, which this round's models no longer have (Open point 6).
- **Each model's changes are measured together** (v0.4 §17.11, v0.5 §18.10, v0.6 §19.11). Packed against Control measures all of v0.4's changes at once; each factored model against Packed measures its factoring, storage and reduction together.
- **Packed-coprime's trial-division bound is 3, below the draw's primes:** 2 and 3 become prime pieces, and 5 and 7 land in pieces treated as composite (5, 7, 25, 35, …), so splits, rewrites and S1's last gcd can occur on this draw, while "base full" cannot (at most 4 pairwise-coprime 7-smooth pieces). Packed-23 against Packed-coprime then compares a fixed $\{2, 3\}$ with an adaptive base over the same two primes plus the matrix's own 5- and 7-pieces. Workloads with larger primes would change both.
- **Every model stays at 64 bits, and overflow is a placeholder mark** (v0.4 §17.10). Marked cells stop early, by the same rule in every model, so they shorten the operation alike; the shares are small (Workloads).
- **The factoring time is not separated from the build's or operation's time.** The counters separate the work.
- **The C round's builds (`mh4.c`, DECISIONS-m4) are not this round's.** They lack the mark, the summation and the row buffer's accounting (v0.4 header); their instruction counts are on m3's draw.
- Results are reported per run, and not combined across runs or with m1–m3.

## Harness

The m3 harness is copied to `m4_*` files. Its spawner, checks, calibration, bootstrap and stops are unchanged (carried over). No m4 runner exists yet (DECISIONS-m4, "For the owner" item 5).

New (coordinator's call M10, pending):
- the five rows, and the build of four models in one language (item 2);
- the generator of v0.4 §17.9, with Python's version pinned (Workloads);
- the simulator for all four models, with the marks step for step and the factored models' lists, bases and factor work;
- the TC gate in v0.4 §17.10's form, with the gate driver's new copy (v0.4 §17.13);
- the adapted call-for-call check of the factored models;
- the six pairings with verdicts, the ratios reported row over reference, and the verdict bounds of item 9;
- the summary tables of item 11 and the overflow tables of item 14;
- the new counters and accounting events;
- this rule's pin.

The harness refuses to write in `../archive/`, in the m1–m3 results, in `nim-pilot/`, or in an earlier m4 run's folder, and checks their sha256 before and after each run (carried over in form). Builder decisions for the round's build and harness go in a new `test/DECISIONS-m4-run.md` (coordinator's call M10), pending the author's review; `test/DECISIONS-m4.md` stays the C build's record.

## The coordinator's calls

| # | Call | Item |
| --- | --- | --- |
| M1 | The null row is Packed's binary, named Packed-null, last of five (**approved by the author, Oct 30, 2026**) | 1 |
| M2 | Ratios reported row over reference, lower is better; time verdicts written as $\text{hi} < 1/(1 + h)$ and $\text{lo} > 1/(1 - h)$, so they equal rule-m3's | 7, 9 |
| M3 | All six pairings carry verdicts, including Packed-23 against Packed-coprime | 7 |
| M4 | Memory keeps rule-m3's 0.90 and 1.10 bands and the 2.0 flag; time keeps h's 5% floor (**approved by the author, Oct 30, 2026**) | 8, 9 |
| M5 | "Range" in the summary is min–max across jobs; per job, the 95% bootstrap interval | 11 |
| M6 | In each pairing the first-named model is the row and the second the reference; Packed against Packed-null is the null pairing, with no verdict | 7 |
| M7 | One build language and setting for all four models; the driver shared; pins and diffs checked language-neutrally | The rows, 2 |
| M8 | "No division test": no division job set in the timed round; the correctness tests stay, the large trial-division unit tests included (**approved by the author, Oct 30, 2026**) | 3, Run plan |
| M9 | Gcd work: an exact check for Packed and the null row; reported, not checked, for the factored models | 7 |
| M10 | Results folders `test/results-m4-*/`; harness files `m4_*`; builder decisions in `test/DECISIONS-m4-run.md` | Run plan, Harness |
| M11 | The overflow tables' form: per job, model, operation and step, with a check column and a per-set summary beside v0.4's simulated bounds | 14 |
| M12 | The pilot's jobs 1, 6 and 10 and rule-m3's projection, over the five rows | Run plan |
| M13 | A new generator `test/m4_gen.py`; step 0's sha256s recorded at the add run and checked by every later run | Workloads |
| M14 | "Base full" at the gates stops the run before step 6; after the gates, the job is reported failed, with no verdict | 12 |

## Open points

1. **The calls still pending:** M2, M3, M5–M7 and M9–M14, as written above.
2. **v0.6 is still being finished.** It changed while this rule was drafted (item 11, S2), and the trial-division bound is being changed from 7 to 3. Some of its sections disagree as of now: §19.4 writes prime pieces with $\phi = 3$, while §19.13's audits and §19.17's instructions say $\phi = 2$ for every piece; C6 and §19.5 rewrite in every step, while §19.15 item 5, §19.16 and §19.17 still say add steps do not rewrite; §19.15 still lists items 2–10 as open, while the header says they are settled. This rule cites v0.6 only by section. It must be rechecked against v0.6's final text before it is pinned, in particular what the pass after the pack and reconciliation put in measures 1–5, §19.13's counters, audits and tests, and which routine the large trial-division unit tests exercise.
3. **v0.5's C1–C10 and v0.6's calls are pending the author's confirmation.** This rule depends on them where it says what is timed (the pass after the pack in measure 1, reconciliation in the operation) and what is checked.
4. **The build.** `spec/nim/` holds Control and Packed only (`control.nim`, `packed.nim`); Packed-23 and Packed-coprime are not built, and no m4 runner exists. The Nim build's pin (version, hash) and its sanitizer build are still to be recorded. Nothing here can be trialled until they exist.
5. **The runtime estimate is untested.** It rests on m3's C timings, instruction-count ratios on m3's draw, a rough draw simulation, and judged multipliers for the factored models. The pilot's projection is the guard.
6. **The grid** (A1, for the author's information only). m3 dropped $n = 128$, and $\rho = 16$ in sop, because of the factored rows' trial division and refinement. This round's models do neither, and the estimate leaves margins of at least 2.0. If $n = 128$ or sop's $\rho = 16$ is wanted later, it would need its own estimate; this rule keeps A1's grid.

## Instructions to builders and reviewers

> **Build the round's harness as m3's, copied to `m4_*`, with the five rows of item 1, and nothing else.**
> - Build all four models in one language, compiler and setting, in four builds each and a sanitizer build, and pin them.
> - Generate the workloads by v0.4 §17.9 with the pinned Python; record their sha256 and check them every run.
> - Simulate all four models, the marks step for step included.
> - Gate every row and job as item 3 says; run the TC gate in v0.4 §17.10's form.
> - Time as rule-m3, every number unchanged; keep the driver's scan for marks outside the clock.
> - Report every ratio row over reference, every verdict with its magnitude, the six pairings, the summary and the overflow tables.
> - Write nowhere but this round's folders.

A reviewer MUST check:
- the five rows, in order, the null row Packed's binary;
- one build setting for all four models, and every pin of item 2;
- step 0's files against their recorded sha256, and the Python version;
- the gates, the TC gate's form on jobs with marks, and the adapted call-for-call check;
- every timing number carried over, and the projection's form;
- the parts: the factored models' pass inside measure 1 and the operation, reconciliation inside the operation, the scan outside the clock;
- the ratios row over reference, and item 9's verdict bounds, equal to rule-m3's;
- the overflow counts per job, model, operation and step, equal across models;
- no write outside the round's folders.
