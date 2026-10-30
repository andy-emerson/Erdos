# Design 3, layer 1: results

Run started 2026-10-29T09:51:30-05:00.

## The measurement rule (research/rule.md, verbatim, written before the first trial)

sha256 `99f3c189c3393bf0e89bf163faf7354d2d472c7b1e5c317a0e006a9ba7d063a1`

<!-- rule begins -->
# Design 3, layer 1: pre-registered measurement rule

Oct 29, 2026 · **Fixed.** Every threshold, count and limit below is **approved by the author, Oct 29, 2026**, and so is every workload parameter, including the 60-minute hard stop, the 45-minute pilot limit and the 2.0 severe-memory-loss flag. The rule is fixed before any measurement: the harness writes it verbatim into its results file before the first trial, and refuses a measured run unless this file holds that text (as 08 harness R-n, by sha256). A change after that is a revision, dated and explained at the top, and allowed only while nothing has been measured.

*Revised Oct 29, 2026, after the first review and at the author's direction:* a second Design 3 row, **L1-opt** (spec §17), with its own exact checks; gcd and multiplication ratios per case, and an exact check that case `den` and the general case do the control's work (item 6); library holder arrays charged for their stored elements only (item 6); the hard stop set to **60 minutes**, and the pilot's projection limit to 45 (Run plan); the severe-memory-loss threshold justified (item 8); keeping every result reported as a limitation on chain jobs (Limitations).

*Revised Oct 29, 2026, after the independent review, at the author's direction; procedure only, no threshold, count, measure or report changes. Nothing has been measured: only the pilot has run, and its results are discarded (Run plan step 3).* **Idle machine.** Before any timed run (the pilot, and each job's calibration and rounds in step 6), the harness checks that the machine is idle: the 1-minute load average at most **0.5**, and no other process using more than **5%** of one core over a 1-second sample. If it is not, the harness re-checks every 15 seconds, logging each check, and after **10 minutes** refuses to time and stops the run, reported to the author. The wait counts on the run's own clock. This is the check added to 08 at the same time; it makes item 4's "nothing else runs during a timed invocation" checked rather than assumed.

*Revised Oct 29, 2026, after the measured run of the same day (commit `b3b88b6`), as an **exception approved by the author (Oct 29, 2026)** to the clause above that allows a revision "only while nothing has been measured".* The revision adds two rows, **L1-Q32** and **L1-opt-Q32** (spec §18), to item 1, with the gates, checks and reports below; the rows measured before are named **L1-Q64** and **L1-opt-Q64** where both widths are meant. No threshold, band, measure, count or limit of this rule changes; the pilot's estimate of the remaining work is updated for the added rows (Within the hard stop, below). The exception carries the author's **safeguards**:
1. **The official Q64 verdicts stay those of commit `b3b88b6`:** L1 and L1-opt against ctl, fmpq, mpq, the best library and fz, and L1-opt against L1, with every item that run reported.
2. **The re-run Q64 rows are used only for the paired Q32-vs-Q64 comparison and for the band h of the same run.** Every row is re-run, so that each Q32 row is timed in the same rounds as its Q64 row. *Documented necessity, not a change to this safeguard:* the band h of item 7 (L1 against L1-null) bands the time verdicts of the rounds it comes from, so it must come from the same rounds as those verdicts; the band h of `b3b88b6` cannot serve the new run. Every time verdict of the new run, the Q32 rows' included, uses the new run's own h. *(This wording is pending the author's confirmation.)*
3. **Every Q64 verdict that changes between the two runs is reported**, with both magnitudes, without choosing between them.
4. **The report states that the Q32 rows were designed after the Q64 results had been seen.**

The revision's further provisions:
- **Capacity.** A Q32 row runs (gates, counting and memory runs, calibration and rounds) only on the jobs whose generator-predicted distinct whole-number count at the end, $W$, is at most **32,768**, the Q32 capacity (spec §18.1 item 2: indices 0 to 32,767). On the others it is reported as **over capacity**: a finding about the design at that width, not n/a for a bug, with the job's $W$ beside it. A Q32 row that reaches its capacity on a job predicted to fit is a bug (item 5). By the generator's figures (deterministic, and reported at `b3b88b6`), jobs 5, 6 and 7 are over capacity ($W$ = 40,849, 40,798 and 50,795), so the Q32 rows run on 8 of the 11 jobs; **jobs 1, 8 and 9 fit but are near capacity** ($W$ = 29,866, 27,881 and 28,379: 91%, 85% and 87% of it).
- **Q32 against Q64.** Per job within capacity, L1-Q32 against L1-Q64 and L1-opt-Q32 against L1-opt-Q64, on time (item 6, with its 95% interval and the job's $h$ of item 7) and on bytes per rational, B_used and B_heap (after entry and at the end), with the verdicts of item 8. The Q32 rows are also compared, like L1 and L1-opt, with ctl, fmpq, mpq, the best library and fz, on time, B_used and B_heap. Their gcd work, multiplications and lookups equal their Q64 rows' (spec §18.3), and are reported as checked, not as a comparison. **Gcd work against time** (item 12) is reported for each Q32 row against ctl: its gcd work and multiplications against ctl's (the row's own counting run, by the fixed bands of item 8), beside its time verdict against ctl in the new run; L1's and L1-opt's stay official at `b3b88b6`.
- **Lookup source, and the lookup gate.** L1-Q64 uses 08's lookup (`sk_lookup` in `lib/vendor08/common.c`, unchanged, its sha256 checked as item 2), and L1-opt-Q64 its own O-6 lookup (spec §17.1); both Q64 rows are compiled from source whose object code is identical, section for section, to that measured at `b3b88b6`, so the Q64 binaries are the ones measured. The Q32 rows use a slot-width-parameterized version of the same algorithm (spec §18.1 item 4). **Same calling layout at both widths:** L1-Q32's lookup is compiled as its own translation unit (`lib/src/plk.c`) and called from `l1.c` as external functions, exactly as L1-Q64 calls `sk_lookup_find` and `sk_lookup_insert` in `common.c`, never inlined into `l1.c` (no link-time optimization), so the L1 pair differs in slot width and not in inlining; L1-opt-Q32 keeps the O-6 lookup inside `l1.c`, as L1-opt-Q64 does. **Lookup gate:** that version, built at 32-bit slots into L1 (the lookup gate's build, never timed), MUST give on every job, in a gate run, probe counts identical to `sk_lookup`'s: every counter equal to L1-Q64's gate run, lookup searches and probes per phase and part included, with the same stored values and whole-number list (item 3).
- **The model's prediction.** Beside every Q32 and Q64 B_used, the report gives the job's $W$, $N$ and $S$, and the model's bits per rational,
  $$\text{bits per rational} = H + E \cdot W/N, \qquad E = 64 + b_s \cdot S/W,$$
  with $H$ the rational's field bits (32 for Q32, 64 for Q64) and $b_s$ the bits per lookup slot (L1-Q64 32, L1-opt-Q64 64, L1-Q32 16, L1-opt-Q32 48: a 16-bit index and the 32-bit fingerprint, spec §18.1 item 5). At a half-full lookup, $S = 2W$, this is the author's $E = 64 + 32$ for L1-Q32 and $64 + 64$ for L1-Q64. **$S/W$ is not fixed:** $S$ is the smallest power of two, at least 16, with $2W \le S$, so $S/W$ lies in $[2, 4)$ once $S$ exceeds 16 ($W \ge 9$), and is exactly 2 when $W$ is a power of two of at least 8; while $S$ is 16 ($1 \le W \le 8$), $S/W = 16/W$, which exceeds 4 when $W < 4$ and reaches 16 at $W = 1$. Over every $W \ge 1$ the range is $[2, 16]$; on this rule's jobs, after entry and at the end, it is $[2, 3.28]$. $E$ is stated with the actual $S$. $W$ comes from the generator and $S$ from the growth rule, both before any run. **For B_used the formula is an accounting identity:** it restates the sum of the memory report (spec §13.2, §18.1 item 6), so B_used equals it to the byte by construction, and the check tests the report, not the design. **Only B_heap tests anything:** it is measured from the C library's heap, with the capacity slack and allocator headers the model leaves out, and is set beside the prediction with their ratio.
- **Within the hard stop.** The rows added make the run about a third longer (the Oct 29 run took 7.6 minutes). **The pilot's projection is updated for the longer run** (Run plan step 3): the remaining work's upper estimate is **11.5 minutes** (gates, counting and memory runs 4.1: 33 gate runs become 60, and 115 counting and memory runs, repeats included, become 155; calibration and rounds 7.4: 69 timed invocations per round become 85), and the pilot's nominal wall time is **1.2 minutes** (its invocations per round go from 31 to 37, the Q32 rows running on pilot jobs 1, 3 and 10). The stop rule itself, the 45-minute projection limit and the 60-minute hard stop are unchanged.
- **The official data.** The results of `b3b88b6` (`results.md`, `verdicts.csv`, `rounds.csv`, `counters.csv`, and the raw `state.json` and `run.log`) are archived, unchanged, in [`test/results-q64-b3b88b6/`](../test/results-q64-b3b88b6/results.md), committed with the code; the harness records each file's sha256, refuses any run while one differs, and never writes, deletes or overwrites a file there or in the original `test/results/`. The run under this revision writes only to **`test/results-q32/`**. The changed-verdict list of safeguard 3 reads the official verdicts from the archive.

It states how layer 1 of [spec v0.1](../v0.1.md), as the baseline L1 and as the optimized variant L1-opt, is measured against the control of 08, FLINT `fmpq`, GMP `mpq_t` and a FLINT shared-denominator ceiling, and how the results are reported: **per measure only, with no overall winner** (item 11).

**This comparison does not end the debate.** L1 is a faithful baseline of the paper's combined structure, with every later layer and every optimization left out on purpose (spec §11); L1-opt adds only the production tricks of spec §17, within the author's exclusions, so it still has no later layer. At 64 bits a gcd is cheap (07 M3 found the bookkeeping of sharing outweighing it there), so a gcd removed here is worth less than it will be at larger sizes; the gcd-work and multiplication counts are reported for that reason.

## Measurement rule

1. **Rows.** Each job runs these rows:
   - **L1**: layer 1, the baseline (spec §§4–15);
   - **L1-opt**: layer 1 with the tricks of spec §17 that the author adopts, every one on; a separate binary, run on the same workloads;
   - **L1-Q32** and **L1-opt-Q32**: L1 and L1-opt with 16-bit index fields (spec §18), each a separate binary from the same source, only on the jobs within their capacity of 32,768 whole numbers (revision of Oct 29, 2026, above); their time verdicts use L1's null spread, as L1-opt's do (item 7);
   - **L1-null**: a second copy of L1, the same binary run as a separate row (the null control);
   - **ctl**: the 08 control, plain reduced rationals (spec §9);
   - **fmpq**: FLINT rationals, and **mpq**: GMP rationals, each in its best normal form (spec §10; the form is calibrated by item 4);
   - **fz**: the FLINT shared-denominator ceiling, only on the jobs marked "fz" in the run plan, which are fully shared: every value is entered over one denominator and every operation is an addition or subtraction, so every result stays over it (as M3's fz).

2. **Builds.** L1, L1-opt, ctl and the driver are built by one compiler with one set of flags (`gcc -std=c11 -O2`), at width 64, with the 08 integer module and the 08 control unchanged (their sha256 recorded, as 08 harness R-o). FLINT and GMP are the system libraries as installed; their versions and `flint_get_num_threads()` (which MUST be 1) are written into every output line and the results file. The timing build carries no counters, audits or allocation hooks. The **counting build** gives the counts and memory reports of L1, L1-opt and ctl, from one complete run per row and job; in it the driver reads every row's counters after every call, untimed, so that each operation's counts can be attributed to its predicted case (spec §14.1 item 3). The **memory run** (item 6) gives every row's heap figures. Counts and memory figures are deterministic: on jobs 3, 5 and 10 every counting run and every memory run is repeated, and the second MUST give identical figures, or it is a bug (item 5). Every counting or memory run has a limit of **10 minutes**; one that does not finish gives n/a, with the reason. The test build gives the audits. L1-null shares L1's gate, counting and memory runs, being the same binary. *(Revision of Oct 29, 2026.)* L1-Q32 and L1-opt-Q32 are built from L1's source with `-DL1_Q32` (spec §18.2), by the same compiler and flags, in the same three builds, and have their counting and memory runs (repeated on jobs 3 and 10, where they run) on the jobs within capacity. The lookup gate's build (L1 with `-DL1_LKGATE`, spec §18.1 item 4) is a test build only, run in the gates, never timed. The Q64 rows' object code is that measured at `b3b88b6` (revision, above).

3. **Gates before the measured run.** In the test build, L1, L1-opt and ctl MUST pass their unit tests (spec §14.3 item 1 and §17.3; 08's for ctl) and, on every job, the gate of spec §14.3 item 2. L1's and L1-opt's gate runs MUST also pass the two exact checks of spec §14.1 items 1 and 2: W against the reference, after entry and at the end, and the count of every case against the generator's prediction. L1-opt's MUST also pass the checks of spec §17.3 items 1–3: its stored values and whole-number list equal to L1's, every predicted trick counter (O-1 skipped lookups per case, and O-2 to O-5 if adopted) equal to the prediction, and its lookups and arithmetic counts equal to L1's less its counted skips. The counting runs MUST pass the check of spec §14.1 item 3: on cases `den` and `general`, L1's arithmetic equal to ctl's. The library rows have no gate, but every one of their runs is checked (item 5). A row that fails a gate is fixed and the gate re-run; no measured run starts until every gate passes. Gates are untimed and MAY run two at a time, one per core, never beside a timed invocation. *(Revision of Oct 29, 2026.)* L1-Q32 and L1-opt-Q32 MUST pass the unit tests at their width (spec §18.3 item 2, with the capacity tests), and, on every job within capacity, the gate and exact checks of L1 and of L1-opt respectively (L1-opt-Q32's §17.3 checks against L1-Q32), and spec §18.3 item 1: their stored values, whole-number list and every counter equal to their Q64 row's, in the gate runs and in the counting runs. **The lookup gate** (revision, above) MUST pass on every job.

4. **Timing.**
   - **Rounds.** Each job runs **12 interleaved rounds**. In every round each row runs once, as its own process, in an order rotated by one row per round, so the rows of a round are paired. Each invocation reads the job's workload file, then runs its reps.
   - **A rep** runs the whole job **r** times in sequence, each on a new, empty class (for the libraries, a new holder array), destroyed untimed after its read-out. The rep's time is its total divided by r. **r** is the smallest power of two for which one rep of that row lasts at least **50 ms**.
   - **Reps per invocation.** If the calibration's rep lasts at least **1 s**, the row runs 1 rep per invocation. Otherwise it runs **3 reps**, the first is discarded, and its time is the minimum of the other 2.
   - **Calibration**, per job, immediately before its round 1, in the timing build, on core 1, recorded in the results file and then fixed for all 12 rounds:
     - **form** (fmpq and mpq): 3 invocations of each form of spec §10 item 1, alternating A, B, A, B, A, B, each at r = 1 with 3 reps (the first discarded); the form with the lower median total is the row's form for the job;
     - **r**, for L1, L1-opt, ctl, fmpq, mpq and fz (in its chosen form for the libraries), as 08 rule item 4: one invocation timing a rep at r = 1, 2, 4, … until one lasts at least 50 ms;
     - L1-null uses L1's r and number of reps. Calibration times enter no measure.
   - **The time** of a row in a round is its **total**: entry + operations + read-out of every stored value as a numerator and denominator in lowest terms. Entry, operations and read-out are also reported. Creating and destroying a class, allocating and freeing a holder array, form B's untimed initialisation, and reading the workload are not timed.
   - **One timed row at a time.** Every timed invocation (calibration, rounds, the timed pilot) runs pinned to **core 1**, one at a time; the harness runs pinned to core 0 and only starts invocations, waits and records. Nothing else runs during a timed invocation. Jobs run in job order, each with its calibration and all 12 rounds before the next.
   - **Time cap.** A timed rep lasting more than **10 s** ends that row's timing on that job: the row is **over cap**, a reported finding, and is not timed again on that job.

5. **Checks, and bugs after the gates.** Every stored value of every invocation of every row (rounds, calibration, counting and memory runs) is compared with the exact value computed by Python's `fractions` from the workload file. The workloads make overflow impossible (Size bound). After the gates:
   - **L1, L1-opt or ctl:** a check failure, an overflow, a failed exact check of spec §14.1 or §17.3, or a non-repeating count is a **bug**, handled as in 08 rule item 5: the row's measured run stops; the bug is fixed under review (spec §12) and logged with the job, invocation, operation and step; the row is re-run in full (gates, counting and memory runs, calibration and every round of every job; L1 together with L1-null; a fix to L1's shared source re-runs L1-opt too). A fix to the 08 module goes through 08's change log and re-runs L1, L1-opt and ctl.
   - **fmpq, mpq or fz:** the driver is reviewed first. A driver bug is fixed, logged, and the row re-run in full. A wrong result that is not the driver's makes the row **n/a** on that job, reported with the value.
   - The report lists every bug beside the per-stratum tables.
   - *(Revision of Oct 29, 2026.)* **L1-Q32 or L1-opt-Q32** are handled as L1 and L1-opt: a check failure, an overflow, a failed exact check of spec §14.1, §17.3 or §18.3, a non-repeating count, or reaching capacity on a job predicted to fit, is a bug. A fix to the shared source `l1.c` re-runs all four layer-1 rows and the lookup gate.

6. **Measures**, per job and row, for L1 against each other row Y (ctl, fmpq, mpq, fz), and likewise for L1-opt in the place of L1, against the same rows and against L1 itself. Below, "L1" stands for either.
   - **Time.** **T(Y) / T(L1)**, T the median of the row's per-round totals; above 1, L1 is faster. Its 95% interval [lo, hi] is the 2.5th–97.5th percentile of the same ratio over **10,000 bootstrap resamples** of the rounds (the same draw for both rows, so the pairing is kept; seed 20261029).
   - **Memory, accounted (B_used).** **B_used(L1) / B_used(Y)**, bytes per stored rational:
     - L1 and ctl: the used bytes of their memory reports (spec §13.2; 08 spec §12.2);
     - fmpq, mpq, fz: the holder array's **stored elements** at their element size (16 bytes per `fmpq`, 32 per `mpq_t`, 8 per `fmpz`, and fz's one $D$), that is, the element size times the number of values stored so far, not the array's full capacity of $N$ elements, plus the **requested bytes of every live heap block the library allocated**, counted by allocator hooks: GMP's `mp_set_memory_functions` and FLINT's `__flint_set_memory_functions`, both installed before the first library call. GMP passes block sizes on reallocation and release; FLINT does not, so the hooks keep a table from block to requested size, held in memory obtained directly from the system with `mmap`, outside the heap that is measured. The hooks call the C library's allocator unchanged.
   - **Memory, heap (B_heap).** **B_heap(L1) / B_heap(Y)**, bytes per stored rational, by one method for every row: the growth of the C library's in-use heap (`mallinfo2`: `uordblks + hblkhd`) from just before the job's first allocation (`create`, or the holder array) to the measurement point. It includes capacity slack, allocator headers and library caches, which B_used leaves out for L1 and ctl. **The holder array is pro-rated:** it is allocated whole, at $N$ elements, before entry, so for fmpq, mpq and fz the bytes of its elements not yet stored (element size times $N$ minus the values stored so far) are subtracted from the heap growth. Without this, after entry (10,000 of 30,000 values stored) a library would be charged about three times its holder bytes; at the end every element is stored and nothing is subtracted. As in M3, `mallopt` sets `M_MMAP_THRESHOLD` and `M_TRIM_THRESHOLD` to $2^{30}$ in every row.
   - Both memory figures come from one **memory run** per row and job: one complete run of the timing build (form A for the libraries, so that an element exists only once its value is stored), with the hooks installed in the library rows only. They are taken **after entry** (after the last entry) and **at the end** (after the last operation, before read-out). Verdicts use the end figures; the after-entry figures are reported beside them.
   - **Untracked allocation.** In each library memory run the hooks also sum `malloc_usable_size` of the live blocks. If B_heap exceeds the holder array's stored elements plus those usable sizes by more than **16 bytes per live block plus 64 KiB**, the run is flagged **untracked allocation**: some memory bypassed the hooks, and B_used undercounts that row. The flag is reported beside both memory verdicts; it is not a bug.
   - **Gcd work** (against ctl, and L1-opt against L1). **G(L1) / G(ctl)**, G the counting build's sum, over every gcd call of entry and operations, of the bit length of the larger operand (08 rule item 6).
   - **Multiplications** (against ctl, and L1-opt against L1). **M(L1) / M(ctl)**, M the counting build's full-width multiplications over entry and operations.
   - **Per case** (L1 against ctl, and L1-opt against L1). The same two ratios, restricted to the operations of each case of spec §7, per operation (add and subtract, multiply, divide): each operation's counts are attributed to the case the generator predicted for it (spec §14.1 item 3). A ratio whose two counts are both 0 is reported as "0 against 0". On cases `den` and `general`, L1 against ctl is 1 exactly, by the exact check of spec §14.1 item 3: **the control already takes the `den` shortcut by comparing values**, so `den` saves no gcd and no multiplication against it (spec §8). These per-case ratios are reported, with no verdict; the verdicts of item 8 use the whole-job ratios.
   - **Lookups** (L1-opt against L1 only). **K(L1-opt) / K(L1)**, K the counting build's lookup searches over entry and operations, with the skipped lookups of each trick beside it.
   - The libraries cannot be counted: their gcd work, multiplications and lookups are **n/a**.

7. **Null control.** L1 against L1-null on the same job, by 6. The job's **null spread** is max(1 − lo, hi − 1, 0); the time band's half-width is **h = max(5%, null spread)**. L1-opt has no null copy of its own: its time verdicts use the same h, since it is built by the same compiler, run by the same driver, and timed in the same rounds.

8. **Verdict per job and measure, with its magnitude**, for L1 against each Y:

   | Measure | L1 wins | L1 loses | Tie | Magnitude reported |
   | --- | --- | --- | --- | --- |
   | Time | lo > 1 + h | hi < 1 − h | otherwise | T(Y) / T(L1), [lo, hi], h |
   | Memory, B_used (end) | ratio ≤ 0.90 | ratio ≥ 1.10 | otherwise | the ratio, after entry and at the end |
   | Memory, B_heap (end) | ratio ≤ 0.90 | ratio ≥ 1.10 | otherwise | the ratio, after entry and at the end |
   | Gcd work (ctl; also L1, for L1-opt) | ratio ≤ 0.90 | ratio ≥ 1.10 | otherwise | G(L1) / G(Y), and per case |
   | Multiplications (ctl; also L1, for L1-opt) | ratio ≤ 0.90 | ratio ≥ 1.10 | otherwise | M(L1) / M(Y), and per case |
   | Lookups (L1-opt against L1) | ratio ≤ 0.90 | ratio ≥ 1.10 | otherwise | K(L1-opt) / K(L1), and the skips per trick |

   A memory ratio ≥ **2.0** at the end raises the **severe-memory-loss flag**, reported beside the verdict; a flag, not a veto. A row over cap has the time verdict **over cap** (L1 over cap: every time verdict of the job is "L1 over cap"). A measure that is n/a has no verdict. Every verdict is reported together with its magnitude; a verdict alone is never reported. Against fz, the verdicts are a **ceiling gap**: how far L1 is from the best possible on fully shared data.

   *Why 2.0, not 08's 4.0 (approved by the author, Oct 29, 2026).* In 08 the flag was set at 4.0 to expose Design 1's pair table, which grows with the square of the number of denominators and so has no upper bound per rational. Layer 1 has one: a rational adds at most two whole numbers, so $W \le 2N$, and with the lookup at $S \le 4W$ slots, $B_{\text{used}}(\text{L1}) = 8 + (8W + 4S)/N \le 8 + 24 \cdot 2 = 56$ bytes per rational. Against ctl's 16 bytes, and fmpq's 16 at 64 bits (an `fmpz` below $2^{62}$ is held inside its element, with no heap block), that is at most 3.5: **a 4.0 flag could never fire on B_used against ctl or fmpq, on any job.** At 2.0 it fires when L1 takes at least twice their memory, $8 + (8W + 4S)/N \ge 32$, that is when there are about one to one and a half distinct whole numbers per rational ($W/N$ from $24/24$ to $24/16$, as $S$ runs from $4W$ down to $2W$): the regime in which interning stores more whole numbers than rationals, which is what the flag is meant to expose. Against mpq (32 bytes per element and two limb blocks) it cannot fire on B_used at either threshold; on B_heap, capacity slack can carry any row past both. If the author prefers 08's 4.0 for consistency, the flag then fires only through B_heap, against fz (8 bytes per value), or through L1-opt with fingerprints (spec §17, O-6: at most $8 + 40 \cdot 2 = 88$ bytes, 5.5 times ctl).

9. **Best library.** Per job, the report also gives L1's and L1-opt's time verdicts against the faster of fmpq and mpq (the one with the lower T), as M3 item 6 did. Taking the minimum of two rows biases this verdict toward the libraries; each library's own verdict is always given beside it.

10. **Strata.** The jobs fall into five strata: **MONEY** (jobs 1–2), **SMALL** (3–4), **MOD** (5–6), **NEW** (7) and **CHAINS** (8–11). For each stratum, each comparison (L1 and L1-opt, each against ctl, fmpq, mpq, best library and fz; L1-opt against L1) and each measure, the report gives the counts of wins, ties, losses, over-cap and n/a results, the median ratio over the stratum's jobs, and the number of severe-memory-loss flags. The per-job verdicts and magnitudes are listed beneath. *(Revision of Oct 29, 2026.)* The Q32 comparisons (L1-Q32 and L1-opt-Q32, each against its Q64 row, ctl, fmpq, mpq, the best library and fz) are given per stratum likewise, with **the count of over-capacity jobs**, in all and per stratum. **The Q32 strata cover fewer jobs** than the Q64 strata: an over-capacity job enters no win, tie, loss, over-cap or n/a count and no median, so by the generator's figures MOD and NEW have no job within capacity, and MONEY, SMALL and CHAINS keep all theirs.

11. **No overall winner.** The rule combines no measures, scores no strata, and names no row as the better one. A narrow win on one measure and a large loss on another are not comparable by counting; choosing is left to discussion of the per-measure results.

12. **Also reported:**
    - **Realized reuse, per job** (from the generator, before any run, and equal to L1's figures by the exact check of spec §14.1): the stored rationals N, the distinct whole numbers W, the **distinct whole numbers per rational W/N**, and the places per whole number 2N/W, each after entry and at the end; the share of operations that met each case of spec §7, per operation; the realized share of operands that are earlier results.
    - **Memory against reuse.** The jobs sorted by W/N at the end, with B_used and B_heap of every row. L1's B_used is checked against $8 + (8W + 4S)/N$ from its own report. *Non-normative:* since the lookup holds between $2W$ and $4W$ slots, L1 takes $8 + 16$ to $8 + 24$ bytes per whole number per rational, so it matches ctl's 16 bytes per rational when W/N is between about 1/3 and 1/2, and uses less only below that.
    - **Counts per operation**, L1, L1-opt and ctl: gcds, gcd work, multiplications, additions, divisions; L1's and L1-opt's lookups (searches, probes), whole numbers found and appended, index comparisons, and cases; and every 08 counter, per phase, core and store, in a CSV.
    - **Per case**, per job and operation: the number of operations in each case, and L1's and ctl's gcds, gcd work and multiplications on them, with the ratios of item 6. The `den` and `general` rows are 1 exactly (spec §14.1 item 3), so the table shows directly which of the other five cases carries each saving against the control.
    - **L1-opt's tricks**, per job: each trick's counters (spec §17.2), its predicted figures where spec §17.3 item 2 gives them, and the share of L1's lookups, gcds, divisions and multiplications that it removes.
    - **Operation time alone**, entry excluded, with verdicts by 8 using the job's h.
    - **Entry against operations**, per row: the share of the total time spent in entry, and, for L1 and L1-opt, the share of gcd work and lookups in `store`.
    - **The limitation of keeping every result** (Limitations), beside every chain verdict against fmpq, mpq and the best library.
    - **Gcd work against time**: a job where L1 removes gcd work or multiplications but not time, at 64 bits, is a candidate for a wider-number test, where both cost more.
    - **The chosen forms** of fmpq and mpq per job, with their calibration medians.

## Workloads

All workloads are written to files by one generator (Python, seeded), with their exact references and predicted figures (item 12), before any measured run. **Master seed 20261029**; job n uses seed 20261029 + n. The 08 generator ([`../../08-skeletons/test/gen.py`](../../08-skeletons/test/gen.py)) is the starting point, and its decisions G-a, G-c, G-d and G-h carry over.

**Inputs.** Every input is an integer pair $(m, d)$ with $d > 0$, given to every row as it is: layer 1 and ctl reduce it in `enter`, fmpq and mpq canonicalise it, fz scales it to $D$. Money is given in cents over 100, unreduced.

**Operations, G3 of 08.** Every job but the chains has $N_{\text{in}} = 10{,}000$ entered values, then $n = 20{,}000$ operations drawn by 08 rule G3 with $\rho = 1/2$: half are **result operations**, whose first operand is a uniformly drawn result of an earlier two-input operation and whose second is an entered value; the rest are two-input operations on entered values drawn uniformly with replacement; the first operation is a two-input one. So $N = 30{,}000$ stored rationals per job. A mix names which operations are drawn, each with equal probability.

**Size bound.** For a value $x = m/d$ in lowest terms write $H(x) = \max(|m|, d)$. Every operation of every job keeps the 08 operand rule $H(x) \cdot H(y) \le (2^{31} - 1)^2$, which bounds every intermediate of ctl and of layer 1 below $2^{63} - 1$ (spec §8.4), so **no overflow can occur**. The generator checks the rule on every operation and stops on a violation (08 harness G-h).

### F1. Money columns: one shared denominator (jobs 1, 2, 8)

- **Amounts.** Cents over 100. After reduction, every denominator is a divisor of 100, so the "one shared denominator" of the data becomes up to nine whole numbers in layer 1, which keeps every value in lowest terms (spec §11); the shared-denominator case meets only operands whose reduced denominators agree. That is part of what is measured.
- **MONEY-wide** (job 1): amounts with a numerator uniform and nonzero in $(-10^{7},\ 10^{7})$ cents; mix add, sub. Most numerators are new.
- **MONEY-catalog** (job 2): a catalog of **200** distinct prices, uniform in $[1,\ 10^{5})$ cents; each amount is a price drawn uniformly, with a uniform sign; mix add, sub. Entered whole numbers repeat; results are mostly new.
- Bounds: $H < 2^{24}$ entered and $H < 2^{25}$ for results of two entered values, so $H(x) H(y) < 2^{49}$.

### F2. Multiplication-heavy work over a small set (jobs 3, 4)

- A set $S_k$ of $k$ whole numbers: $1$ and $k - 1$ distinct integers uniform in $[2,\ 2^{15})$.
- Each entered value is $\pm a/b$ with $a$ and $b$ drawn uniformly from $S_k$, redrawn together until $\gcd(a, b) = 1$, with a uniform sign. Values repeat.
- **SMALL8** (job 3): $k = 8$; mix mul, div. **SMALL64** (job 4): $k = 64$; mix mul, div.
- Bounds: entered $H < 2^{15}$, results of two entered values $H < 2^{30}$, so $H(x) H(y) < 2^{45}$.

### F3. Mixed operations with moderate reuse (jobs 5, 6)

- A pool $P$ of **1,000** distinct whole numbers drawn as 08's P50 denominators (08 rule G1, σ = 0.5, B = 20), so pool members share small factors and the general cases make real gcds.
- Each entered value is $\pm a/b$ with $a \ne b$ drawn uniformly from $P$, redrawn together until $\gcd(a, b) = 1$, with a uniform sign. A whole number can be a numerator of one value and a denominator of another.
- **MOD-mixed** (job 5): mix add, sub, mul, div. **MOD-muldiv** (job 6): mix mul, div.
- Bounds: as the 08 matrix at B = 20 (entered $H < 2^{20}$).

### F4. Additions that keep producing new numerators (job 7)

- **NEW-add** (job 7): the 08 P50-add job's construction (08 rule G1 and G2: σ = 0.5, B = 20, D = 1,000 denominators, V = 10 values each, numerators uniform in $[-(2^{20} - 1),\ 2^{20} - 1]$ coprime to their denominator), with this rule's seed; mix add. Nearly every sum brings a new numerator, and most bring a new denominator: **the honest worst case for layer 1.**

### F5. Chains: results feed operations (jobs 8–11)

1,000 chains, run round-robin, so that every step's first operand is the chain's previous result: $x_k = x_{k-1} \circ y_k$, with $x_0$ and every $y_k$ drawn uniformly from the 10,000 entered inputs. Every step's result is stored and kept (spec §10 item 3). **Every chain is bounded by construction**, with no redraw and no restart (see Lessons).

- **CH-money** (job 8; fz): K = 32 steps of add or sub; inputs as MONEY-catalog. $|x_k| < 33 \cdot 10^{5}$ cents, so $H < 2^{22}$.
- **CH-closed** (job 9): 08 rule G5 CH-closed exactly (K = 32 steps of add or sub; inputs over uniformly drawn divisors of $L = 720{,}720$, $|m| < d$, coprime as 08 harness G-f); every denominator divides L.
- **CH-small** (job 10): K = 8 steps of mul or div; inputs $\pm a/b$ with $a$, $b$ uniform in $[1, 63]$, redrawn together until coprime, uniform sign. Each step adds at most 6 bits: $H(x_{k-1}) H(y) < 2^{6k + 6} \le 2^{54}$.
- **CH-mixed** (job 11): as CH-small, but each step is add, sub, mul or div. Each step adds at most 7 bits: $H(x_{k-1}) H(y) < 2^{7k + 5} \le 2^{61}$. No input is zero, so no divisor is zero; a sum may be zero and is then stored as $0/1$.

## Run plan

Width 64. Rows L1, L1-opt, L1-null, ctl, fmpq and mpq on every job; fz where marked; *(revision of Oct 29, 2026)* L1-Q32 and L1-opt-Q32 on the jobs within the Q32 capacity, marked "Q32" with the generator's $W$ at the end (over capacity: jobs 5, 6 and 7; near capacity: jobs 1, 8 and 9); and the lookup gate on every job, in step 4.

| Job | Name | Family | Parameters | fz | Q32 ($W$ at the end) |
| --- | --- | --- | --- | --- | --- |
| 1 | MONEY-wide | F1 | amounts uniform in $(-10^7, 10^7)$ cents; add, sub | yes | yes (29,866; near capacity) |
| 2 | MONEY-catalog | F1 | 200 prices; add, sub | yes | yes (15,403) |
| 3 | SMALL8 | F2 | $k = 8$; mul, div | | yes (350) |
| 4 | SMALL64 | F2 | $k = 64$; mul, div | | yes (21,346) |
| 5 | MOD-mixed | F3 | pool of 1,000; add, sub, mul, div | | over capacity (40,849) |
| 6 | MOD-muldiv | F3 | pool of 1,000; mul, div | | over capacity (40,798) |
| 7 | NEW-add | F4 | 08 P50-add construction; add | | over capacity (50,795) |
| 8 | CH-money | F5 | K = 32, add, sub; catalog inputs | yes | yes (27,881; near capacity) |
| 9 | CH-closed | F5 | 08 G5 CH-closed | | yes (28,379; near capacity) |
| 10 | CH-small | F5 | K = 8, mul, div; whole numbers below 64 | | yes (10,945) |
| 11 | CH-mixed | F5 | K = 8, four operations; whole numbers below 64 | | yes (12,682) |

The jobs span reuse from high (SMALL8, CH-closed, CH-small) through moderate (SMALL64, MOD, MONEY-catalog) to low (MONEY-wide, NEW-add); the realized figures are reported by item 12.

**Order of work.**
1. Generate the workloads, the exact references and the predicted figures.
2. Unit tests of L1 and ctl, in the test build.
3. **Pilot**: jobs 1, 3, 5, 7 and 10, every row, in the timing build, with form and r calibration and **2 rounds**, one timed invocation at a time on core 1. Its results and calibration are discarded. **Stop rule:** the harness projects the whole run as the time elapsed so far, plus the remaining work's upper estimate below (8.5 minutes: gates, counting and memory runs 2.5, calibration and rounds 6) times the larger of 1 and the pilot's wall time over **1 minute**. *(Revision of Oct 29, 2026: with the Q32 rows, 11.5 minutes and 1.2 minutes; see the revision at the top.)* **If the projection exceeds 45 minutes, the run stops and is reported to the author before anything else runs.** The limit is set below the 60-minute hard stop so that a run projected to fit keeps a quarter of its time for re-runs under item 5.
4. Gates (item 3), two at a time on both cores; with the revision of Oct 29, 2026, also the Q32 rows' gates on the jobs within capacity and the lookup gate on every job.
5. Counting runs and memory runs (items 2 and 6), two at a time on both cores.
6. Calibration and the 12 rounds (item 4), job by job, one timed invocation at a time on core 1, with nothing else running.

**Hard stop at 60 minutes.** The whole run, from the start of step 1 to the end of the last round, re-runs under item 5 included, has a hard limit of **60 minutes** of the run's own wall time, summed over its sessions; time spent fixing a bug under review, between sessions, is not counted (08 harness R-m). When it is reached the harness stops at once, writes what it has, marked **stopped**, with the step and job, and reports to the author; no verdict is reported from a stopped run.

*Non-normative estimate,* to be confirmed by the pilot. A 64-bit gcd costs 30–60 ns.
- **One complete run of a job** (30,000 stored values): ctl 5–20 ms (08's estimate for the same size); L1 10–40 ms (two lookups per stored value); L1-opt 8–40 ms; fmpq 5–20 ms; mpq 10–40 ms; fz 2–10 ms. So r is 2 to 16, and an invocation (3 reps of 50–100 ms, and process start) takes 0.2–0.35 s.
- **One round:** 69 invocations (11 jobs × 6 rows, and fz on 3), 14–24 s. **Calibration:** 132 form invocations at r = 1 and 69 r-calibrations, about 0.5–1 minute. **Calibration and 12 rounds: 3.5–6 minutes.**
- **Gates:** about 670 full audits per job and row, each linear in N + W + S with one gcd per rational, 1–3 ms: under 1.5 minutes on two cores, with L1-opt. **Counting and memory runs:** under 1 minute, with counters read after every call. **Generation, references and predictions:** 1–3 minutes. **Unit tests and builds:** about 2 minutes. **Pilot:** 0.5–1 minute.
- *(Revision of Oct 29, 2026.)* The Q32 rows add 16 timed invocations per round (two rows on 8 jobs: 69 become 85) and 16 r-calibrations, 16 gate runs and 11 lookup-gate runs (33 gate runs become 60), and 16 counting and 16 memory runs and 8 repeats (115 become 155): gates 2.7 minutes, counting and memory runs 1.4, calibration and rounds 7.4, so the pilot's remaining-work estimate becomes 11.5 minutes (from 8.5), about a third more work.
- **The whole run: about 11 to 20 minutes** with the Q32 rows *(revised Oct 29, 2026, from 9 to 17 before them: the upper estimate gains 1.2 minutes of gates, 0.4 of counting and memory runs, 1.4 of calibration and rounds and 0.2 of pilot, and the lower about a third of its measured work)*. If every row is three times slower than estimated, about 60 minutes: at the 60-minute hard stop, though the pilot's 45-minute projection limit would then stop the run first and report it to the author.

## Limitations, reported with the results

- **Keeping every result** (spec §10 item 3). Every row stores every result as a new element, so that all rows hold the same values and are read out alike. On the chain jobs (8–11) this is not the libraries' normal use: a program that needs only each chain's current value would update it in place (`fmpq_add(x, x, y)`, `mpq_add(x, x, y)`), reusing its limbs, with one element per chain rather than one per step. Keeping every result charges fmpq and mpq for an initialisation and a live element per step. Form B partly offsets this: it moves every initialisation out of the timed phases, so the time charge is mostly removed when calibration chooses it; the memory charge remains. The verdicts against fmpq, mpq and the best library on jobs 8–11 are reported with this note.
- **One row with every trick.** L1-opt is timed with all its adopted tricks on together; the counters show each trick's effect on the work, but no trick's effect on time is measured alone.

## Lessons carried over (non-normative)

From the 08 skeleton run ([results](../../08-skeletons/test/results/results.md), [library](../../08-skeletons/lib/DECISIONS.md) and [harness](../../08-skeletons/test/DECISIONS.md) decisions) and from 07 M3 ([rule](../../07-prime-graph/research/m3-rule.md), [results](../../07-prime-graph/research/m3-results.md)):
- **Chains bounded by construction.** In 08, the redraw-and-restart rule G6 broke the intended share of result operands at full size (realized 0.29, 0.30 and 0.14 in CH-open, CH-muldiv and CH-none, against 0.5). Every chain here is bounded by the size of its inputs, so no step is redrawn and every step's first operand is a result.
- **The 08 pilot projected 0.11 hours** for a run estimated at 25–65 minutes. This run is smaller, so a 45-minute projection limit leaves room; it remains a stop rule, not a target.
- **Ambiguities closed in advance:** the null copy shares its twin's gates, counting and memory runs (08 harness ambiguity 4); the hard-stop clock is the run's own time (08 R-m); a builder who cannot reach the author records choices as pending (spec L-19).
- **Library usage as M3:** single-threaded FLINT, versions recorded, `mallopt` thresholds, initialisation inside timed phases (form A) with the array form (form B) offered by calibration, and the best-library verdict reported beside each library's own.
- **M3 measured no memory.** Here the libraries' memory is measured from their real allocations, through their own allocator hooks and the C library's heap statistics, with a flag when the two disagree.
- **A faithful-implementation check.** The generator predicts, from magnitudes alone, how many operations take each case and how many whole numbers there are; layer 1's counts must match exactly, so a shortcut that is skipped or misapplied is caught as a bug, not hidden in the timings.

## Harness

The 08 harness ([`../../08-skeletons/test/`](../../08-skeletons/test/): `gen.py`, `runner.py`, `analyze.py`, `bench.c`) is the starting point, with the M3 driver ([`../../07-prime-graph/test/m3/m3_bench.c`](../../07-prime-graph/test/m3/m3_bench.c)) for the library rows. Its decisions carry over where they apply: G-a, G-c, G-d, G-h (generator); R-a to R-c, R-g, R-i to R-o (driver and runner, with this rule's rows, times and limits); A-a, A-b, A-g (analysis, with L1-null as the null row). New choices are recorded in `test/DECISIONS.md`, pending the author's review.
<!-- rule ends -->

### Calibration, job 1 (MONEY-wide)

- fmpq: form A (median totals: A 1.702 ms, B 1.709 ms)
- mpq: form B (median totals: A 4.223 ms, B 3.898 ms)

| row | form | r | calibration rep (ms, r runs) | reps per invocation |
| --- | --- | --- | --- | --- |
| L1 | - | 32 | 75.59 | 3 |
| L1-opt | - | 32 | 77.20 | 3 |
| L1-null | - | 32 (L1's) | 75.59 (L1's) | 3 (L1's) |
| L1-Q32 | - | 32 | 74.43 | 3 |
| L1-opt-Q32 | - | 32 | 77.48 | 3 |
| ctl | - | 32 | 51.25 | 3 |
| fmpq | A | 32 | 56.06 | 3 |
| mpq | B | 16 | 63.33 | 3 |
| fz | - | 64 | 87.97 | 3 |

### Calibration, job 2 (MONEY-catalog)

- fmpq: form A (median totals: A 1.462 ms, B 1.489 ms)
- mpq: form B (median totals: A 4.385 ms, B 4.161 ms)

| row | form | r | calibration rep (ms, r runs) | reps per invocation |
| --- | --- | --- | --- | --- |
| L1 | - | 32 | 63.79 | 3 |
| L1-opt | - | 32 | 63.97 | 3 |
| L1-null | - | 32 (L1's) | 63.79 (L1's) | 3 (L1's) |
| L1-Q32 | - | 32 | 62.13 | 3 |
| L1-opt-Q32 | - | 32 | 64.99 | 3 |
| ctl | - | 64 | 88.42 | 3 |
| fmpq | A | 64 | 96.22 | 3 |
| mpq | B | 16 | 67.46 | 3 |
| fz | - | 64 | 90.63 | 3 |

### Calibration, job 3 (SMALL8)

- fmpq: form A (median totals: A 1.529 ms, B 1.568 ms)
- mpq: form B (median totals: A 4.546 ms, B 4.149 ms)

| row | form | r | calibration rep (ms, r runs) | reps per invocation |
| --- | --- | --- | --- | --- |
| L1 | - | 32 | 61.77 | 3 |
| L1-opt | - | 32 | 56.32 | 3 |
| L1-null | - | 32 (L1's) | 61.77 (L1's) | 3 (L1's) |
| L1-Q32 | - | 32 | 60.09 | 3 |
| L1-opt-Q32 | - | 32 | 55.91 | 3 |
| ctl | - | 32 | 53.39 | 3 |
| fmpq | A | 64 | 96.89 | 3 |
| mpq | B | 16 | 65.61 | 3 |

### Calibration, job 4 (SMALL64)

- fmpq: form A (median totals: A 2.249 ms, B 2.250 ms)
- mpq: form B (median totals: A 4.813 ms, B 4.531 ms)

| row | form | r | calibration rep (ms, r runs) | reps per invocation |
| --- | --- | --- | --- | --- |
| L1 | - | 32 | 96.81 | 3 |
| L1-opt | - | 32 | 96.47 | 3 |
| L1-null | - | 32 (L1's) | 96.81 (L1's) | 3 (L1's) |
| L1-Q32 | - | 32 | 93.67 | 3 |
| L1-opt-Q32 | - | 32 | 95.17 | 3 |
| ctl | - | 32 | 65.27 | 3 |
| fmpq | A | 32 | 70.57 | 3 |
| mpq | B | 16 | 71.30 | 3 |

### Calibration, job 5 (MOD-mixed)

- fmpq: form B (median totals: A 2.722 ms, B 2.704 ms)
- mpq: form B (median totals: A 5.255 ms, B 4.756 ms)

| row | form | r | calibration rep (ms, r runs) | reps per invocation |
| --- | --- | --- | --- | --- |
| L1 | - | 16 | 57.00 | 3 |
| L1-opt | - | 16 | 60.22 | 3 |
| L1-null | - | 16 (L1's) | 57.00 (L1's) | 3 (L1's) |
| ctl | - | 32 | 77.50 | 3 |
| fmpq | B | 32 | 87.82 | 3 |
| mpq | B | 16 | 73.21 | 3 |

### Calibration, job 6 (MOD-muldiv)

- fmpq: form B (median totals: A 2.779 ms, B 2.773 ms)
- mpq: form B (median totals: A 5.305 ms, B 5.135 ms)

| row | form | r | calibration rep (ms, r runs) | reps per invocation |
| --- | --- | --- | --- | --- |
| L1 | - | 16 | 58.23 | 3 |
| L1-opt | - | 16 | 62.86 | 3 |
| L1-null | - | 16 (L1's) | 58.23 (L1's) | 3 (L1's) |
| ctl | - | 32 | 80.02 | 3 |
| fmpq | B | 32 | 87.92 | 3 |
| mpq | B | 16 | 81.55 | 3 |

### Calibration, job 7 (NEW-add)

- fmpq: form B (median totals: A 2.539 ms, B 2.521 ms)
- mpq: form B (median totals: A 4.272 ms, B 4.022 ms)

| row | form | r | calibration rep (ms, r runs) | reps per invocation |
| --- | --- | --- | --- | --- |
| L1 | - | 16 | 56.05 | 3 |
| L1-opt | - | 16 | 57.00 | 3 |
| L1-null | - | 16 (L1's) | 56.05 (L1's) | 3 (L1's) |
| ctl | - | 32 | 76.82 | 3 |
| fmpq | B | 32 | 88.62 | 3 |
| mpq | B | 16 | 63.45 | 3 |

### Calibration, job 8 (CH-money)

- fmpq: form B (median totals: A 2.255 ms, B 2.178 ms)
- mpq: form B (median totals: A 6.105 ms, B 5.552 ms)

| row | form | r | calibration rep (ms, r runs) | reps per invocation |
| --- | --- | --- | --- | --- |
| L1 | - | 32 | 96.72 | 3 |
| L1-opt | - | 32 | 96.03 | 3 |
| L1-null | - | 32 (L1's) | 96.72 (L1's) | 3 (L1's) |
| L1-Q32 | - | 32 | 92.32 | 3 |
| L1-opt-Q32 | - | 32 | 97.08 | 3 |
| ctl | - | 32 | 65.66 | 3 |
| fmpq | B | 32 | 69.53 | 3 |
| mpq | B | 8 | 60.39 | 3 |
| fz | - | 32 | 66.52 | 3 |

### Calibration, job 9 (CH-closed)

- fmpq: form B (median totals: A 2.759 ms, B 2.724 ms)
- mpq: form B (median totals: A 6.490 ms, B 6.061 ms)

| row | form | r | calibration rep (ms, r runs) | reps per invocation |
| --- | --- | --- | --- | --- |
| L1 | - | 16 | 55.70 | 3 |
| L1-opt | - | 16 | 54.53 | 3 |
| L1-null | - | 16 (L1's) | 55.70 (L1's) | 3 (L1's) |
| L1-Q32 | - | 16 | 53.87 | 3 |
| L1-opt-Q32 | - | 16 | 54.91 | 3 |
| ctl | - | 32 | 78.33 | 3 |
| fmpq | B | 32 | 88.32 | 3 |
| mpq | B | 16 | 101.07 | 3 |

### Calibration, job 10 (CH-small)

- fmpq: form B (median totals: A 0.773 ms, B 0.766 ms)
- mpq: form B (median totals: A 1.898 ms, B 1.685 ms)

| row | form | r | calibration rep (ms, r runs) | reps per invocation |
| --- | --- | --- | --- | --- |
| L1 | - | 64 | 69.76 | 3 |
| L1-opt | - | 64 | 70.01 | 3 |
| L1-null | - | 64 (L1's) | 69.76 (L1's) | 3 (L1's) |
| L1-Q32 | - | 64 | 68.66 | 3 |
| L1-opt-Q32 | - | 64 | 72.16 | 3 |
| ctl | - | 128 | 92.98 | 3 |
| fmpq | B | 128 | 98.79 | 3 |
| mpq | B | 32 | 56.06 | 3 |

### Calibration, job 11 (CH-mixed)

- fmpq: form B (median totals: A 0.804 ms, B 0.797 ms)
- mpq: form B (median totals: A 1.882 ms, B 1.685 ms)

| row | form | r | calibration rep (ms, r runs) | reps per invocation |
| --- | --- | --- | --- | --- |
| L1 | - | 64 | 70.14 | 3 |
| L1-opt | - | 64 | 68.99 | 3 |
| L1-null | - | 64 (L1's) | 70.14 (L1's) | 3 (L1's) |
| L1-Q32 | - | 64 | 68.36 | 3 |
| L1-opt-Q32 | - | 64 | 71.08 | 3 |
| ctl | - | 128 | 91.85 | 3 |
| fmpq | B | 64 | 51.40 | 3 |
| mpq | B | 32 | 54.39 | 3 |

## Run summary

- started 2026-10-29T09:51:30-05:00; elapsed (run's own clock) 10.9 min
- libraries: version row=fmpq gmp=6.3.0 flint=3.0.1 threads=1 form=A
- 08 copies (lib/vendor08, commit 08bbfbe): sha256 checked before the run
- unit tests unit-layer1: pass (unit (Q64 rows): 1146105 checks, 0 failures)
- unit tests unit-layer1-q32: pass (unit (Q32 rows): 1146105 checks, 0 failures)
- unit tests diff-layer1: pass (checksum 7b6083446efe8a13)
- unit tests diff-layer1-q32: pass (checksum 7b6083446efe8a13)
- unit tests q32-layer1: pass (q32: 1416037 checks, 0 failures)
- unit tests unit-08: pass (unit (width 64): 3379 checks, 0 failures)
- finished 2026-10-29T10:02:25-05:00
- gates: 60 of 60 passed

## Pilot (rule, Run plan step 3; discarded)

Jobs [1, 3, 5, 7, 10], every row, 2 rounds. Pilot wall time 22.2 s (factor 1.00); elapsed at the end of the pilot 1.24 min; **projected whole run 12.7 min** (limit 45 min).

Median per-run total over the pilot's rounds (ms), with the calibrated r and form:

| job | L1 | L1-opt | L1-null | L1-Q32 | L1-opt-Q32 | ctl | fmpq | mpq | fz |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | 2.36 (r=32) | 2.40 (r=32) | 2.37 (r=32) | 2.28 (r=32) | 2.44 (r=32) | 1.59 (r=32) | 1.72 (r=32 B) | 4.18 (r=16 B) | 1.37 (r=64) |
| 3 | 1.90 (r=32) | 1.76 (r=32) | 1.89 (r=32) | 1.90 (r=32) | 1.77 (r=32) | 1.69 (r=32) | 1.53 (r=64 A) | 4.28 (r=16 B) | - |
| 5 | 3.56 (r=16) | 3.73 (r=16) | 3.55 (r=16) | - | - | 2.40 (r=32) | 2.70 (r=32 B) | 4.81 (r=16 B) | - |
| 7 | 3.43 (r=16) | 3.66 (r=16) | 3.45 (r=16) | - | - | 2.26 (r=32) | 2.54 (r=32 A) | 4.12 (r=16 B) | - |
| 10 | 1.09 (r=64) | 1.10 (r=64) | 1.08 (r=64) | 1.06 (r=64) | 1.11 (r=64) | 0.71 (r=128) | 0.77 (r=128 A) | 1.68 (r=32 B) | - |

## Workloads and realized reuse (rule item 12)

N stored rationals, W distinct whole numbers, W/N whole numbers per rational, 2N/W places per whole number; each after entry and at the end.

| job | name | N entry | W entry | W/N entry | 2N/W entry | N end | W end | W/N end | 2N/W end | result operand share |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | MONEY-wide | 10000 | 9991 | 0.999 | 2.00 | 30000 | 29866 | 0.996 | 2.01 | 0.249 |
| 2 | MONEY-catalog | 10000 | 209 | 0.021 | 95.69 | 30000 | 15403 | 0.513 | 3.90 | 0.251 |
| 3 | SMALL8 | 10000 | 8 | 0.001 | 2500.00 | 30000 | 350 | 0.012 | 171.43 | 0.249 |
| 4 | SMALL64 | 10000 | 64 | 0.006 | 312.50 | 30000 | 21346 | 0.712 | 2.81 | 0.252 |
| 5 | MOD-mixed | 10000 | 1000 | 0.100 | 20.00 | 30000 | 40849 | 1.362 | 1.47 | 0.252 |
| 6 | MOD-muldiv | 10000 | 1000 | 0.100 | 20.00 | 30000 | 40798 | 1.360 | 1.47 | 0.250 |
| 7 | NEW-add | 10000 | 10929 | 1.093 | 1.83 | 30000 | 50795 | 1.693 | 1.18 | 0.250 |
| 8 | CH-money | 10000 | 208 | 0.021 | 96.15 | 42000 | 27881 | 0.664 | 3.01 | 0.484 |
| 9 | CH-closed | 10000 | 3268 | 0.327 | 6.12 | 42000 | 28379 | 0.676 | 2.96 | 0.484 |
| 10 | CH-small | 10000 | 63 | 0.006 | 317.46 | 18000 | 10945 | 0.608 | 3.29 | 0.438 |
| 11 | CH-mixed | 10000 | 63 | 0.006 | 317.46 | 18000 | 12682 | 0.705 | 2.84 | 0.438 |

Share of operations meeting each case (spec §7), per operation:

- job 1 (MONEY-wide): add: den 0.243, general 0.757; sub: den 0.253, general 0.747
- job 2 (MONEY-catalog): add: den 0.257, general 0.743; sub: den 0.245, general 0.755
- job 3 (SMALL8): mul: both 0.018, den 0.109, num 0.087, cross_ad 0.085, cross_cb 0.077, general 0.625; div: both 0.019, cross_ad 0.108, cross_cb 0.087, num 0.078, den 0.079, general 0.629
- job 4 (SMALL64): mul: both 0.000, den 0.010, num 0.010, cross_ad 0.008, cross_cb 0.010, general 0.961; div: both 0.000, cross_ad 0.008, cross_cb 0.011, num 0.010, den 0.009, general 0.963
- job 5 (MOD-mixed): add: cross 0.001, den 0.000, num 0.001, general 0.998; sub: cross 0.000, den 0.001, num 0.000, general 0.999; mul: den 0.001, num 0.000, cross_ad 0.001, cross_cb 0.001, general 0.997; div: cross_ad 0.000, cross_cb 0.000, den 0.000, general 0.999
- job 6 (MOD-muldiv): mul: den 0.000, num 0.001, cross_ad 0.000, cross_cb 0.001, general 0.998; div: cross_ad 0.001, cross_cb 0.000, num 0.001, den 0.001, general 0.998
- job 7 (NEW-add): add: den 0.000, general 1.000
- job 8 (CH-money): add: den 0.237, general 0.763; sub: den 0.243, general 0.757
- job 9 (CH-closed): add: cross 0.000, den 0.006, num 0.000, general 0.994; sub: cross 0.000, den 0.004, num 0.000, general 0.996
- job 10 (CH-small): mul: den 0.003, num 0.002, cross_ad 0.002, cross_cb 0.002, general 0.990; div: cross_ad 0.004, cross_cb 0.002, num 0.003, den 0.004, general 0.988
- job 11 (CH-mixed): add: cross 0.004, den 0.002, num 0.002, general 0.992; sub: cross 0.002, den 0.004, num 0.003, general 0.992; mul: both 0.001, den 0.004, num 0.003, cross_ad 0.005, cross_cb 0.002, general 0.984; div: cross_ad 0.002, cross_cb 0.002, num 0.002, den 0.003, general 0.991

## Verdicts of the Q32 rows (rule items 8-10 and the revision of Oct 29, 2026; per measure only, no overall winner)

**The Q32 rows were designed after the Q64 results of commit b3b88b6 had been seen (rule, revision of Oct 29, 2026: an exception approved by the author to "only while nothing has been measured"). The official Q64 verdicts, against ctl and the libraries and of L1-opt against L1, stay those of b3b88b6 ([archived results](../results-q64-b3b88b6/results.md)); the Q64 rows re-run here are used only for the paired Q32-against-Q64 comparison and for the band h of the same run (L1 against L1-null, which must come from the same rounds as the verdicts it bands), and every Q64 verdict that differs between the two runs is listed below, without choosing between them.**

*Over capacity:* 3 of 11 jobs (job 5, W = 40849, job 6, W = 40798, job 7, W = 50795) have more than 32,768 distinct whole numbers at the end; the Q32 rows do not run there, and each of their verdicts on those jobs is **over capacity**, a finding about the design at that width.

*Limitation:* L1-opt is timed with all its adopted tricks on together; the counters show each trick's effect on the work, but no trick's effect on time is measured alone (rule, Limitations: one row with every trick).

### Strata (rule item 10)

**Over-capacity jobs: 3** of 11 (5, 6, 7). **The Q32 strata cover fewer jobs** than the Q64 strata of b3b88b6: a stratum's over-capacity jobs are counted in the "over capacity" column and enter no other count or median.

| stratum | jobs | over capacity | X | Y | measure | win | tie | loss | over cap | n/a | median ratio | severe flags |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| MONEY | 2 | 0 | L1-Q32 | L1 | B_heap | 2 | 0 | 0 | 0 | 0 | 0.652 | 0 |
| MONEY | 2 | 0 | L1-Q32 | L1 | B_used | 2 | 0 | 0 | 0 | 0 | 0.643 | 0 |
| MONEY | 2 | 0 | L1-Q32 | L1 | time | 0 | 2 | 0 | 0 | 0 | 1.031 | 0 |
| MONEY | 2 | 0 | L1-Q32 | best | time | 0 | 0 | 2 | 0 | 0 | 0.762 | 0 |
| MONEY | 2 | 0 | L1-Q32 | ctl | B_heap | 1 | 1 | 0 | 0 | 0 | 0.825 | 0 |
| MONEY | 2 | 0 | L1-Q32 | ctl | B_used | 1 | 1 | 0 | 0 | 0 | 0.832 | 0 |
| MONEY | 2 | 0 | L1-Q32 | ctl | time | 0 | 0 | 2 | 0 | 0 | 0.704 | 0 |
| MONEY | 2 | 0 | L1-Q32 | fmpq | B_heap | 1 | 0 | 1 | 0 | 0 | 0.901 | 0 |
| MONEY | 2 | 0 | L1-Q32 | fmpq | B_used | 1 | 1 | 0 | 0 | 0 | 0.832 | 0 |
| MONEY | 2 | 0 | L1-Q32 | fmpq | time | 0 | 0 | 2 | 0 | 0 | 0.762 | 0 |
| MONEY | 2 | 0 | L1-Q32 | fz | B_heap | 0 | 0 | 2 | 0 | 0 | 1.786 | 1 |
| MONEY | 2 | 0 | L1-Q32 | fz | B_used | 0 | 0 | 2 | 0 | 0 | 1.663 | 1 |
| MONEY | 2 | 0 | L1-Q32 | fz | time | 0 | 0 | 2 | 0 | 0 | 0.667 | 0 |
| MONEY | 2 | 0 | L1-Q32 | mpq | B_heap | 2 | 0 | 0 | 0 | 0 | 0.151 | 0 |
| MONEY | 2 | 0 | L1-Q32 | mpq | B_used | 2 | 0 | 0 | 0 | 0 | 0.249 | 0 |
| MONEY | 2 | 0 | L1-Q32 | mpq | time | 2 | 0 | 0 | 0 | 0 | 1.930 | 0 |
| MONEY | 2 | 0 | L1-opt-Q32 | L1-opt | B_heap | 2 | 0 | 0 | 0 | 0 | 0.734 | 0 |
| MONEY | 2 | 0 | L1-opt-Q32 | L1-opt | B_used | 2 | 0 | 0 | 0 | 0 | 0.727 | 0 |
| MONEY | 2 | 0 | L1-opt-Q32 | L1-opt | time | 0 | 2 | 0 | 0 | 0 | 0.989 | 0 |
| MONEY | 2 | 0 | L1-opt-Q32 | best | time | 0 | 0 | 2 | 0 | 0 | 0.726 | 0 |
| MONEY | 2 | 0 | L1-opt-Q32 | ctl | B_heap | 1 | 0 | 1 | 0 | 0 | 1.172 | 0 |
| MONEY | 2 | 0 | L1-opt-Q32 | ctl | B_used | 0 | 1 | 1 | 0 | 0 | 1.242 | 0 |
| MONEY | 2 | 0 | L1-opt-Q32 | ctl | time | 0 | 0 | 2 | 0 | 0 | 0.670 | 0 |
| MONEY | 2 | 0 | L1-opt-Q32 | fmpq | B_heap | 0 | 1 | 1 | 0 | 0 | 1.279 | 0 |
| MONEY | 2 | 0 | L1-opt-Q32 | fmpq | B_used | 0 | 1 | 1 | 0 | 0 | 1.241 | 0 |
| MONEY | 2 | 0 | L1-opt-Q32 | fmpq | time | 0 | 0 | 2 | 0 | 0 | 0.726 | 0 |
| MONEY | 2 | 0 | L1-opt-Q32 | fz | B_heap | 0 | 0 | 2 | 0 | 0 | 2.536 | 1 |
| MONEY | 2 | 0 | L1-opt-Q32 | fz | B_used | 0 | 0 | 2 | 0 | 0 | 2.482 | 1 |
| MONEY | 2 | 0 | L1-opt-Q32 | fz | time | 0 | 0 | 2 | 0 | 0 | 0.636 | 0 |
| MONEY | 2 | 0 | L1-opt-Q32 | mpq | B_heap | 2 | 0 | 0 | 0 | 0 | 0.215 | 0 |
| MONEY | 2 | 0 | L1-opt-Q32 | mpq | B_used | 2 | 0 | 0 | 0 | 0 | 0.371 | 0 |
| MONEY | 2 | 0 | L1-opt-Q32 | mpq | time | 2 | 0 | 0 | 0 | 0 | 1.840 | 0 |
| SMALL | 2 | 0 | L1-Q32 | L1 | B_heap | 2 | 0 | 0 | 0 | 0 | 0.596 | 0 |
| SMALL | 2 | 0 | L1-Q32 | L1 | B_used | 2 | 0 | 0 | 0 | 0 | 0.566 | 0 |
| SMALL | 2 | 0 | L1-Q32 | L1 | time | 0 | 2 | 0 | 0 | 0 | 1.009 | 0 |
| SMALL | 2 | 0 | L1-Q32 | best | time | 0 | 0 | 2 | 0 | 0 | 0.782 | 0 |
| SMALL | 2 | 0 | L1-Q32 | ctl | B_heap | 1 | 1 | 0 | 0 | 0 | 0.642 | 0 |
| SMALL | 2 | 0 | L1-Q32 | ctl | B_used | 2 | 0 | 0 | 0 | 0 | 0.569 | 0 |
| SMALL | 2 | 0 | L1-Q32 | ctl | time | 0 | 0 | 2 | 0 | 0 | 0.797 | 0 |
| SMALL | 2 | 0 | L1-Q32 | fmpq | B_heap | 1 | 0 | 1 | 0 | 0 | 0.701 | 0 |
| SMALL | 2 | 0 | L1-Q32 | fmpq | B_used | 2 | 0 | 0 | 0 | 0 | 0.569 | 0 |
| SMALL | 2 | 0 | L1-Q32 | fmpq | time | 0 | 0 | 2 | 0 | 0 | 0.782 | 0 |
| SMALL | 2 | 0 | L1-Q32 | mpq | B_heap | 2 | 0 | 0 | 0 | 0 | 0.118 | 0 |
| SMALL | 2 | 0 | L1-Q32 | mpq | B_used | 2 | 0 | 0 | 0 | 0 | 0.155 | 0 |
| SMALL | 2 | 0 | L1-Q32 | mpq | time | 2 | 0 | 0 | 0 | 0 | 1.853 | 0 |
| SMALL | 2 | 0 | L1-opt-Q32 | L1-opt | B_heap | 2 | 0 | 0 | 0 | 0 | 0.645 | 0 |
| SMALL | 2 | 0 | L1-opt-Q32 | L1-opt | B_used | 2 | 0 | 0 | 0 | 0 | 0.623 | 0 |
| SMALL | 2 | 0 | L1-opt-Q32 | L1-opt | time | 0 | 2 | 0 | 0 | 0 | 1.002 | 0 |
| SMALL | 2 | 0 | L1-opt-Q32 | best | time | 0 | 0 | 2 | 0 | 0 | 0.802 | 0 |
| SMALL | 2 | 0 | L1-opt-Q32 | ctl | B_heap | 1 | 0 | 1 | 0 | 0 | 0.871 | 0 |
| SMALL | 2 | 0 | L1-opt-Q32 | ctl | B_used | 1 | 0 | 1 | 0 | 0 | 0.847 | 0 |
| SMALL | 2 | 0 | L1-opt-Q32 | ctl | time | 0 | 1 | 1 | 0 | 0 | 0.821 | 0 |
| SMALL | 2 | 0 | L1-opt-Q32 | fmpq | B_heap | 1 | 0 | 1 | 0 | 0 | 0.951 | 0 |
| SMALL | 2 | 0 | L1-opt-Q32 | fmpq | B_used | 1 | 0 | 1 | 0 | 0 | 0.847 | 0 |
| SMALL | 2 | 0 | L1-opt-Q32 | fmpq | time | 0 | 0 | 2 | 0 | 0 | 0.802 | 0 |
| SMALL | 2 | 0 | L1-opt-Q32 | mpq | B_heap | 2 | 0 | 0 | 0 | 0 | 0.160 | 0 |
| SMALL | 2 | 0 | L1-opt-Q32 | mpq | B_used | 2 | 0 | 0 | 0 | 0 | 0.231 | 0 |
| SMALL | 2 | 0 | L1-opt-Q32 | mpq | time | 2 | 0 | 0 | 0 | 0 | 1.912 | 0 |
| MOD | 2 | 2 | L1-Q32 | L1 | B_heap | 0 | 0 | 0 | 0 | 0 | n/a | 0 |
| MOD | 2 | 2 | L1-Q32 | L1 | B_used | 0 | 0 | 0 | 0 | 0 | n/a | 0 |
| MOD | 2 | 2 | L1-Q32 | L1 | time | 0 | 0 | 0 | 0 | 0 | n/a | 0 |
| MOD | 2 | 2 | L1-Q32 | best | time | 0 | 0 | 0 | 0 | 0 | n/a | 0 |
| MOD | 2 | 2 | L1-Q32 | ctl | B_heap | 0 | 0 | 0 | 0 | 0 | n/a | 0 |
| MOD | 2 | 2 | L1-Q32 | ctl | B_used | 0 | 0 | 0 | 0 | 0 | n/a | 0 |
| MOD | 2 | 2 | L1-Q32 | ctl | time | 0 | 0 | 0 | 0 | 0 | n/a | 0 |
| MOD | 2 | 2 | L1-Q32 | fmpq | B_heap | 0 | 0 | 0 | 0 | 0 | n/a | 0 |
| MOD | 2 | 2 | L1-Q32 | fmpq | B_used | 0 | 0 | 0 | 0 | 0 | n/a | 0 |
| MOD | 2 | 2 | L1-Q32 | fmpq | time | 0 | 0 | 0 | 0 | 0 | n/a | 0 |
| MOD | 2 | 2 | L1-Q32 | mpq | B_heap | 0 | 0 | 0 | 0 | 0 | n/a | 0 |
| MOD | 2 | 2 | L1-Q32 | mpq | B_used | 0 | 0 | 0 | 0 | 0 | n/a | 0 |
| MOD | 2 | 2 | L1-Q32 | mpq | time | 0 | 0 | 0 | 0 | 0 | n/a | 0 |
| MOD | 2 | 2 | L1-opt-Q32 | L1-opt | B_heap | 0 | 0 | 0 | 0 | 0 | n/a | 0 |
| MOD | 2 | 2 | L1-opt-Q32 | L1-opt | B_used | 0 | 0 | 0 | 0 | 0 | n/a | 0 |
| MOD | 2 | 2 | L1-opt-Q32 | L1-opt | time | 0 | 0 | 0 | 0 | 0 | n/a | 0 |
| MOD | 2 | 2 | L1-opt-Q32 | best | time | 0 | 0 | 0 | 0 | 0 | n/a | 0 |
| MOD | 2 | 2 | L1-opt-Q32 | ctl | B_heap | 0 | 0 | 0 | 0 | 0 | n/a | 0 |
| MOD | 2 | 2 | L1-opt-Q32 | ctl | B_used | 0 | 0 | 0 | 0 | 0 | n/a | 0 |
| MOD | 2 | 2 | L1-opt-Q32 | ctl | time | 0 | 0 | 0 | 0 | 0 | n/a | 0 |
| MOD | 2 | 2 | L1-opt-Q32 | fmpq | B_heap | 0 | 0 | 0 | 0 | 0 | n/a | 0 |
| MOD | 2 | 2 | L1-opt-Q32 | fmpq | B_used | 0 | 0 | 0 | 0 | 0 | n/a | 0 |
| MOD | 2 | 2 | L1-opt-Q32 | fmpq | time | 0 | 0 | 0 | 0 | 0 | n/a | 0 |
| MOD | 2 | 2 | L1-opt-Q32 | mpq | B_heap | 0 | 0 | 0 | 0 | 0 | n/a | 0 |
| MOD | 2 | 2 | L1-opt-Q32 | mpq | B_used | 0 | 0 | 0 | 0 | 0 | n/a | 0 |
| MOD | 2 | 2 | L1-opt-Q32 | mpq | time | 0 | 0 | 0 | 0 | 0 | n/a | 0 |
| NEW | 1 | 1 | L1-Q32 | L1 | B_heap | 0 | 0 | 0 | 0 | 0 | n/a | 0 |
| NEW | 1 | 1 | L1-Q32 | L1 | B_used | 0 | 0 | 0 | 0 | 0 | n/a | 0 |
| NEW | 1 | 1 | L1-Q32 | L1 | time | 0 | 0 | 0 | 0 | 0 | n/a | 0 |
| NEW | 1 | 1 | L1-Q32 | best | time | 0 | 0 | 0 | 0 | 0 | n/a | 0 |
| NEW | 1 | 1 | L1-Q32 | ctl | B_heap | 0 | 0 | 0 | 0 | 0 | n/a | 0 |
| NEW | 1 | 1 | L1-Q32 | ctl | B_used | 0 | 0 | 0 | 0 | 0 | n/a | 0 |
| NEW | 1 | 1 | L1-Q32 | ctl | time | 0 | 0 | 0 | 0 | 0 | n/a | 0 |
| NEW | 1 | 1 | L1-Q32 | fmpq | B_heap | 0 | 0 | 0 | 0 | 0 | n/a | 0 |
| NEW | 1 | 1 | L1-Q32 | fmpq | B_used | 0 | 0 | 0 | 0 | 0 | n/a | 0 |
| NEW | 1 | 1 | L1-Q32 | fmpq | time | 0 | 0 | 0 | 0 | 0 | n/a | 0 |
| NEW | 1 | 1 | L1-Q32 | mpq | B_heap | 0 | 0 | 0 | 0 | 0 | n/a | 0 |
| NEW | 1 | 1 | L1-Q32 | mpq | B_used | 0 | 0 | 0 | 0 | 0 | n/a | 0 |
| NEW | 1 | 1 | L1-Q32 | mpq | time | 0 | 0 | 0 | 0 | 0 | n/a | 0 |
| NEW | 1 | 1 | L1-opt-Q32 | L1-opt | B_heap | 0 | 0 | 0 | 0 | 0 | n/a | 0 |
| NEW | 1 | 1 | L1-opt-Q32 | L1-opt | B_used | 0 | 0 | 0 | 0 | 0 | n/a | 0 |
| NEW | 1 | 1 | L1-opt-Q32 | L1-opt | time | 0 | 0 | 0 | 0 | 0 | n/a | 0 |
| NEW | 1 | 1 | L1-opt-Q32 | best | time | 0 | 0 | 0 | 0 | 0 | n/a | 0 |
| NEW | 1 | 1 | L1-opt-Q32 | ctl | B_heap | 0 | 0 | 0 | 0 | 0 | n/a | 0 |
| NEW | 1 | 1 | L1-opt-Q32 | ctl | B_used | 0 | 0 | 0 | 0 | 0 | n/a | 0 |
| NEW | 1 | 1 | L1-opt-Q32 | ctl | time | 0 | 0 | 0 | 0 | 0 | n/a | 0 |
| NEW | 1 | 1 | L1-opt-Q32 | fmpq | B_heap | 0 | 0 | 0 | 0 | 0 | n/a | 0 |
| NEW | 1 | 1 | L1-opt-Q32 | fmpq | B_used | 0 | 0 | 0 | 0 | 0 | n/a | 0 |
| NEW | 1 | 1 | L1-opt-Q32 | fmpq | time | 0 | 0 | 0 | 0 | 0 | n/a | 0 |
| NEW | 1 | 1 | L1-opt-Q32 | mpq | B_heap | 0 | 0 | 0 | 0 | 0 | n/a | 0 |
| NEW | 1 | 1 | L1-opt-Q32 | mpq | B_used | 0 | 0 | 0 | 0 | 0 | n/a | 0 |
| NEW | 1 | 1 | L1-opt-Q32 | mpq | time | 0 | 0 | 0 | 0 | 0 | n/a | 0 |
| CHAINS | 4 | 0 | L1-Q32 | L1 | B_heap | 4 | 0 | 0 | 0 | 0 | 0.629 | 0 |
| CHAINS | 4 | 0 | L1-Q32 | L1 | B_used | 4 | 0 | 0 | 0 | 0 | 0.635 | 0 |
| CHAINS | 4 | 0 | L1-Q32 | L1 | time | 0 | 4 | 0 | 0 | 0 | 1.026 | 0 |
| CHAINS | 4 | 0 | L1-Q32 | best | time | 0 | 0 | 4 | 0 | 0 | 0.750 | 0 |
| CHAINS | 4 | 0 | L1-Q32 | ctl | B_heap | 4 | 0 | 0 | 0 | 0 | 0.634 | 0 |
| CHAINS | 4 | 0 | L1-Q32 | ctl | B_used | 4 | 0 | 0 | 0 | 0 | 0.782 | 0 |
| CHAINS | 4 | 0 | L1-Q32 | ctl | time | 0 | 0 | 4 | 0 | 0 | 0.690 | 0 |
| CHAINS | 4 | 0 | L1-Q32 | fmpq | B_heap | 0 | 2 | 2 | 0 | 0 | 1.068 | 0 |
| CHAINS | 4 | 0 | L1-Q32 | fmpq | B_used | 4 | 0 | 0 | 0 | 0 | 0.782 | 0 |
| CHAINS | 4 | 0 | L1-Q32 | fmpq | time | 0 | 0 | 4 | 0 | 0 | 0.750 | 0 |
| CHAINS | 4 | 0 | L1-Q32 | fz | B_heap | 0 | 0 | 1 | 0 | 0 | 1.956 | 0 |
| CHAINS | 4 | 0 | L1-Q32 | fz | B_used | 0 | 0 | 1 | 0 | 0 | 1.553 | 0 |
| CHAINS | 4 | 0 | L1-Q32 | fz | time | 0 | 0 | 1 | 0 | 0 | 0.708 | 0 |
| CHAINS | 4 | 0 | L1-Q32 | mpq | B_heap | 4 | 0 | 0 | 0 | 0 | 0.180 | 0 |
| CHAINS | 4 | 0 | L1-Q32 | mpq | B_used | 4 | 0 | 0 | 0 | 0 | 0.230 | 0 |
| CHAINS | 4 | 0 | L1-Q32 | mpq | time | 4 | 0 | 0 | 0 | 0 | 1.729 | 0 |
| CHAINS | 4 | 0 | L1-opt-Q32 | L1-opt | B_heap | 4 | 0 | 0 | 0 | 0 | 0.742 | 0 |
| CHAINS | 4 | 0 | L1-opt-Q32 | L1-opt | B_used | 4 | 0 | 0 | 0 | 0 | 0.724 | 0 |
| CHAINS | 4 | 0 | L1-opt-Q32 | L1-opt | time | 0 | 4 | 0 | 0 | 0 | 0.981 | 0 |
| CHAINS | 4 | 0 | L1-opt-Q32 | best | time | 0 | 0 | 4 | 0 | 0 | 0.722 | 0 |
| CHAINS | 4 | 0 | L1-opt-Q32 | ctl | B_heap | 4 | 0 | 0 | 0 | 0 | 0.780 | 0 |
| CHAINS | 4 | 0 | L1-opt-Q32 | ctl | B_used | 0 | 0 | 4 | 0 | 0 | 1.205 | 0 |
| CHAINS | 4 | 0 | L1-opt-Q32 | ctl | time | 0 | 0 | 4 | 0 | 0 | 0.664 | 0 |
| CHAINS | 4 | 0 | L1-opt-Q32 | fmpq | B_heap | 0 | 0 | 4 | 0 | 0 | 1.312 | 0 |
| CHAINS | 4 | 0 | L1-opt-Q32 | fmpq | B_used | 0 | 0 | 4 | 0 | 0 | 1.204 | 0 |
| CHAINS | 4 | 0 | L1-opt-Q32 | fmpq | time | 0 | 0 | 4 | 0 | 0 | 0.722 | 0 |
| CHAINS | 4 | 0 | L1-opt-Q32 | fz | B_heap | 0 | 0 | 1 | 0 | 0 | 2.443 | 1 |
| CHAINS | 4 | 0 | L1-opt-Q32 | fz | B_used | 0 | 0 | 1 | 0 | 0 | 2.333 | 1 |
| CHAINS | 4 | 0 | L1-opt-Q32 | fz | time | 0 | 0 | 1 | 0 | 0 | 0.681 | 0 |
| CHAINS | 4 | 0 | L1-opt-Q32 | mpq | B_heap | 4 | 0 | 0 | 0 | 0 | 0.221 | 0 |
| CHAINS | 4 | 0 | L1-opt-Q32 | mpq | B_used | 4 | 0 | 0 | 0 | 0 | 0.353 | 0 |
| CHAINS | 4 | 0 | L1-opt-Q32 | mpq | time | 4 | 0 | 0 | 0 | 0 | 1.669 | 0 |

### Per job and measure, with magnitudes (rule item 8)

#### Job 1 (MONEY-wide)

Null control L1 against L1-null (this run): ratio 1.000 [0.995, 1.002], h = 0.050

| X | Y | measure | verdict | ratio | interval / after entry | flags |
| --- | --- | --- | --- | --- | --- | --- |
| L1-Q32 | L1 | time | tie | 1.031 | [1.026, 1.035], h 0.050 | paired Q32 against Q64 |
| L1-Q32 | L1 | B_used | win | 0.661 | 0.637 | paired Q32 against Q64 |
| L1-Q32 | L1 | B_heap | win | 0.671 | 0.672 | paired Q32 against Q64 |
| L1-opt-Q32 | L1-opt | time | tie | 0.990 | [0.985, 0.995], h 0.050 | paired Q32 against Q64 |
| L1-opt-Q32 | L1-opt | B_used | win | 0.750 | 0.750 | paired Q32 against Q64 |
| L1-opt-Q32 | L1-opt | B_heap | win | 0.758 | 0.709 | paired Q32 against Q64 |
| L1-Q32 | ctl | time | loss | 0.698 | [0.697, 0.700], h 0.050 |  |
| L1-Q32 | ctl | B_used | tie | 1.021 | 1.159 |  |
| L1-Q32 | ctl | B_heap | tie | 1.012 | 1.024 |  |
| L1-Q32 | fmpq | time | loss | 0.759 | [0.758, 0.761], h 0.050 |  |
| L1-Q32 | fmpq | B_used | tie | 1.021 | 1.158 |  |
| L1-Q32 | fmpq | B_heap | loss | 1.105 | 1.678 |  |
| L1-Q32 | mpq | time | win | 1.731 | [1.722, 1.809], h 0.050 |  |
| L1-Q32 | mpq | B_used | win | 0.305 | 0.386 |  |
| L1-Q32 | mpq | B_heap | win | 0.185 | 0.280 |  |
| L1-Q32 | fz | time | loss | 0.602 | [0.600, 0.605], h 0.050 | ceiling gap |
| L1-Q32 | fz | B_used | loss | 2.041 | 2.314 | severe memory loss, ceiling gap |
| L1-Q32 | fz | B_heap | loss | 2.190 | 3.354 | severe memory loss, ceiling gap |
| L1-Q32 | best | time | loss | 0.759 | [0.758, 0.761], h 0.050 | best library: fmpq |
| L1-opt-Q32 | ctl | time | loss | 0.659 | [0.657, 0.661], h 0.050 |  |
| L1-opt-Q32 | ctl | B_used | loss | 1.567 | 1.978 |  |
| L1-opt-Q32 | ctl | B_heap | loss | 1.482 | 1.721 |  |
| L1-opt-Q32 | fmpq | time | loss | 0.716 | [0.714, 0.719], h 0.050 |  |
| L1-opt-Q32 | fmpq | B_used | loss | 1.567 | 1.977 |  |
| L1-opt-Q32 | fmpq | B_heap | loss | 1.618 | 2.821 |  |
| L1-opt-Q32 | mpq | time | win | 1.633 | [1.623, 1.707], h 0.050 |  |
| L1-opt-Q32 | mpq | B_used | win | 0.468 | 0.659 |  |
| L1-opt-Q32 | mpq | B_heap | win | 0.272 | 0.470 |  |
| L1-opt-Q32 | fz | time | loss | 0.568 | [0.565, 0.570], h 0.050 | ceiling gap |
| L1-opt-Q32 | fz | B_used | loss | 3.132 | 3.950 | severe memory loss, ceiling gap |
| L1-opt-Q32 | fz | B_heap | loss | 3.207 | 5.638 | severe memory loss, ceiling gap |
| L1-opt-Q32 | best | time | loss | 0.716 | [0.714, 0.719], h 0.050 | best library: fmpq |

Operation time alone (entry and read-out excluded; rule item 12, verdicts by item 8 with the job's h): L1-Q32 vs L1 tie 1.028 [1.024, 1.031]; L1-Q32 vs ctl loss 0.717 [0.714, 0.720]; L1-Q32 vs fmpq loss 0.759 [0.757, 0.762]; L1-Q32 vs mpq win 1.852 [1.837, 1.950]; L1-Q32 vs fz loss 0.083 [0.083, 0.083]; L1-opt-Q32 vs L1-opt tie 0.992 [0.989, 0.997]; L1-opt-Q32 vs ctl loss 0.679 [0.676, 0.682]; L1-opt-Q32 vs fmpq loss 0.719 [0.717, 0.722]; L1-opt-Q32 vs mpq win 1.754 [1.739, 1.847]; L1-opt-Q32 vs fz loss 0.079 [0.078, 0.079]

#### Job 2 (MONEY-catalog)

Null control L1 against L1-null (this run): ratio 1.002 [0.998, 1.005], h = 0.050

| X | Y | measure | verdict | ratio | interval / after entry | flags |
| --- | --- | --- | --- | --- | --- | --- |
| L1-Q32 | L1 | time | tie | 1.031 | [1.025, 1.033], h 0.050 | paired Q32 against Q64 |
| L1-Q32 | L1 | B_used | win | 0.625 | 0.510 | paired Q32 against Q64 |
| L1-Q32 | L1 | B_heap | win | 0.633 | 0.523 | paired Q32 against Q64 |
| L1-opt-Q32 | L1-opt | time | tie | 0.988 | [0.982, 0.995], h 0.050 | paired Q32 against Q64 |
| L1-opt-Q32 | L1-opt | B_used | win | 0.703 | 0.522 | paired Q32 against Q64 |
| L1-opt-Q32 | L1-opt | B_heap | win | 0.711 | 0.517 | paired Q32 against Q64 |
| L1-Q32 | ctl | time | loss | 0.709 | [0.706, 0.711], h 0.050 |  |
| L1-Q32 | ctl | B_used | win | 0.643 | 0.267 |  |
| L1-Q32 | ctl | B_heap | win | 0.639 | 0.280 |  |
| L1-Q32 | fmpq | time | loss | 0.766 | [0.762, 0.768], h 0.050 |  |
| L1-Q32 | fmpq | B_used | win | 0.643 | 0.267 |  |
| L1-Q32 | fmpq | B_heap | win | 0.697 | 0.458 |  |
| L1-Q32 | mpq | time | win | 2.128 | [2.111, 2.189], h 0.050 |  |
| L1-Q32 | mpq | B_used | win | 0.192 | 0.089 |  |
| L1-Q32 | mpq | B_heap | win | 0.117 | 0.076 |  |
| L1-Q32 | fz | time | loss | 0.732 | [0.729, 0.735], h 0.050 | ceiling gap |
| L1-Q32 | fz | B_used | loss | 1.286 | 0.533 | ceiling gap |
| L1-Q32 | fz | B_heap | loss | 1.383 | 0.916 | ceiling gap |
| L1-Q32 | best | time | loss | 0.766 | [0.762, 0.768], h 0.050 | best library: fmpq |
| L1-opt-Q32 | ctl | time | loss | 0.681 | [0.678, 0.687], h 0.050 |  |
| L1-opt-Q32 | ctl | B_used | tie | 0.916 | 0.280 |  |
| L1-opt-Q32 | ctl | B_heap | win | 0.862 | 0.491 |  |
| L1-opt-Q32 | fmpq | time | loss | 0.736 | [0.731, 0.739], h 0.050 |  |
| L1-opt-Q32 | fmpq | B_used | tie | 0.916 | 0.279 |  |
| L1-opt-Q32 | fmpq | B_heap | tie | 0.941 | 0.805 |  |
| L1-opt-Q32 | mpq | time | win | 2.046 | [2.028, 2.116], h 0.050 |  |
| L1-opt-Q32 | mpq | B_used | win | 0.274 | 0.093 |  |
| L1-opt-Q32 | mpq | B_heap | win | 0.158 | 0.134 |  |
| L1-opt-Q32 | fz | time | loss | 0.703 | [0.700, 0.707], h 0.050 | ceiling gap |
| L1-opt-Q32 | fz | B_used | loss | 1.832 | 0.558 | ceiling gap |
| L1-opt-Q32 | fz | B_heap | loss | 1.865 | 1.609 | ceiling gap |
| L1-opt-Q32 | best | time | loss | 0.736 | [0.731, 0.739], h 0.050 | best library: fmpq |

Operation time alone (entry and read-out excluded; rule item 12, verdicts by item 8 with the job's h): L1-Q32 vs L1 tie 1.033 [1.026, 1.035]; L1-Q32 vs ctl loss 0.695 [0.692, 0.698]; L1-Q32 vs fmpq loss 0.727 [0.722, 0.729]; L1-Q32 vs mpq win 2.152 [2.121, 2.214]; L1-Q32 vs fz loss 0.091 [0.090, 0.091]; L1-opt-Q32 vs L1-opt tie 0.982 [0.975, 0.988]; L1-opt-Q32 vs ctl loss 0.665 [0.661, 0.670]; L1-opt-Q32 vs fmpq loss 0.696 [0.691, 0.700]; L1-opt-Q32 vs mpq win 2.060 [2.024, 2.135]; L1-opt-Q32 vs fz loss 0.087 [0.086, 0.088]

#### Job 3 (SMALL8)

Null control L1 against L1-null (this run): ratio 1.000 [0.994, 1.002], h = 0.050

| X | Y | measure | verdict | ratio | interval / after entry | flags |
| --- | --- | --- | --- | --- | --- | --- |
| L1-Q32 | L1 | time | tie | 1.000 | [0.996, 1.009], h 0.050 | paired Q32 against Q64 |
| L1-Q32 | L1 | B_used | win | 0.506 | 0.500 | paired Q32 against Q64 |
| L1-Q32 | L1 | B_heap | win | 0.522 | 0.501 | paired Q32 against Q64 |
| L1-opt-Q32 | L1-opt | time | tie | 1.001 | [0.991, 1.010], h 0.050 | paired Q32 against Q64 |
| L1-opt-Q32 | L1-opt | B_used | win | 0.514 | 0.501 | paired Q32 against Q64 |
| L1-opt-Q32 | L1-opt | B_heap | win | 0.532 | 0.501 | paired Q32 against Q64 |
| L1-Q32 | ctl | time | loss | 0.890 | [0.886, 0.896], h 0.050 |  |
| L1-Q32 | ctl | B_used | win | 0.260 | 0.251 |  |
| L1-Q32 | ctl | B_heap | win | 0.275 | 0.251 |  |
| L1-Q32 | fmpq | time | loss | 0.807 | [0.802, 0.811], h 0.050 |  |
| L1-Q32 | fmpq | B_used | win | 0.260 | 0.250 |  |
| L1-Q32 | fmpq | B_heap | win | 0.301 | 0.412 |  |
| L1-Q32 | mpq | time | win | 2.148 | [2.128, 2.174], h 0.050 |  |
| L1-Q32 | mpq | B_used | win | 0.071 | 0.084 |  |
| L1-Q32 | mpq | B_heap | win | 0.050 | 0.069 |  |
| L1-Q32 | best | time | loss | 0.807 | [0.802, 0.811], h 0.050 | best library: fmpq |
| L1-opt-Q32 | ctl | time | tie | 0.954 | [0.945, 0.958], h 0.050 |  |
| L1-opt-Q32 | ctl | B_used | win | 0.269 | 0.251 |  |
| L1-opt-Q32 | ctl | B_heap | win | 0.261 | 0.459 |  |
| L1-opt-Q32 | fmpq | time | loss | 0.864 | [0.855, 0.868], h 0.050 |  |
| L1-opt-Q32 | fmpq | B_used | win | 0.269 | 0.251 |  |
| L1-opt-Q32 | fmpq | B_heap | win | 0.285 | 0.752 |  |
| L1-opt-Q32 | mpq | time | win | 2.302 | [2.269, 2.323], h 0.050 |  |
| L1-opt-Q32 | mpq | B_used | win | 0.073 | 0.084 |  |
| L1-opt-Q32 | mpq | B_heap | win | 0.048 | 0.125 |  |
| L1-opt-Q32 | best | time | loss | 0.864 | [0.855, 0.868], h 0.050 | best library: fmpq |

Operation time alone (entry and read-out excluded; rule item 12, verdicts by item 8 with the job's h): L1-Q32 vs L1 tie 1.002 [0.997, 1.010]; L1-Q32 vs ctl tie 0.987 [0.982, 0.990]; L1-Q32 vs fmpq loss 0.874 [0.870, 0.878]; L1-Q32 vs mpq win 2.367 [2.342, 2.400]; L1-opt-Q32 vs L1-opt tie 1.010 [1.003, 1.019]; L1-opt-Q32 vs ctl tie 1.056 [1.049, 1.059]; L1-opt-Q32 vs fmpq loss 0.936 [0.930, 0.939]; L1-opt-Q32 vs mpq win 2.534 [2.509, 2.567]

#### Job 4 (SMALL64)

Null control L1 against L1-null (this run): ratio 1.001 [0.998, 1.003], h = 0.050

| X | Y | measure | verdict | ratio | interval / after entry | flags |
| --- | --- | --- | --- | --- | --- | --- |
| L1-Q32 | L1 | time | tie | 1.019 | [1.013, 1.022], h 0.050 | paired Q32 against Q64 |
| L1-Q32 | L1 | B_used | win | 0.627 | 0.503 | paired Q32 against Q64 |
| L1-Q32 | L1 | B_heap | win | 0.669 | 0.500 | paired Q32 against Q64 |
| L1-opt-Q32 | L1-opt | time | tie | 1.002 | [0.999, 1.008], h 0.050 | paired Q32 against Q64 |
| L1-opt-Q32 | L1-opt | B_used | win | 0.731 | 0.506 | paired Q32 against Q64 |
| L1-opt-Q32 | L1-opt | B_heap | win | 0.758 | 0.504 | paired Q32 against Q64 |
| L1-Q32 | ctl | time | loss | 0.704 | [0.702, 0.706], h 0.050 |  |
| L1-Q32 | ctl | B_used | win | 0.879 | 0.255 |  |
| L1-Q32 | ctl | B_heap | tie | 1.009 | 0.258 |  |
| L1-Q32 | fmpq | time | loss | 0.757 | [0.754, 0.759], h 0.050 |  |
| L1-Q32 | fmpq | B_used | win | 0.879 | 0.255 |  |
| L1-Q32 | fmpq | B_heap | loss | 1.101 | 0.422 |  |
| L1-Q32 | mpq | time | win | 1.559 | [1.534, 1.572], h 0.050 |  |
| L1-Q32 | mpq | B_used | win | 0.240 | 0.085 |  |
| L1-Q32 | mpq | B_heap | win | 0.185 | 0.070 |  |
| L1-Q32 | best | time | loss | 0.757 | [0.754, 0.759], h 0.050 | best library: fmpq |
| L1-opt-Q32 | ctl | time | loss | 0.688 | [0.686, 0.691], h 0.050 |  |
| L1-opt-Q32 | ctl | B_used | loss | 1.425 | 0.258 |  |
| L1-opt-Q32 | ctl | B_heap | loss | 1.482 | 0.467 |  |
| L1-opt-Q32 | fmpq | time | loss | 0.740 | [0.738, 0.742], h 0.050 |  |
| L1-opt-Q32 | fmpq | B_used | loss | 1.425 | 0.258 |  |
| L1-opt-Q32 | fmpq | B_heap | loss | 1.618 | 0.766 |  |
| L1-opt-Q32 | mpq | time | win | 1.523 | [1.498, 1.536], h 0.050 |  |
| L1-opt-Q32 | mpq | B_used | win | 0.389 | 0.086 |  |
| L1-opt-Q32 | mpq | B_heap | win | 0.272 | 0.128 |  |
| L1-opt-Q32 | best | time | loss | 0.740 | [0.738, 0.742], h 0.050 | best library: fmpq |

Operation time alone (entry and read-out excluded; rule item 12, verdicts by item 8 with the job's h): L1-Q32 vs L1 tie 1.020 [1.015, 1.023]; L1-Q32 vs ctl loss 0.712 [0.710, 0.714]; L1-Q32 vs fmpq loss 0.752 [0.749, 0.754]; L1-Q32 vs mpq win 1.544 [1.520, 1.566]; L1-opt-Q32 vs L1-opt tie 1.004 [1.001, 1.009]; L1-opt-Q32 vs ctl loss 0.681 [0.680, 0.684]; L1-opt-Q32 vs fmpq loss 0.719 [0.717, 0.722]; L1-opt-Q32 vs mpq win 1.477 [1.452, 1.497]

#### Job 5 (MOD-mixed)

Null control L1 against L1-null (this run): ratio 0.996 [0.987, 1.007], h = 0.050

L1-Q32 and L1-opt-Q32: **over capacity** (W = 40849 > 32,768); every Q32 verdict of this job is over capacity.

| X | Y | measure | verdict | ratio | interval / after entry | flags |
| --- | --- | --- | --- | --- | --- | --- |
| L1-Q32 | L1 | time | over capacity | n/a | W = 40849 > 32768 | paired Q32 against Q64 |
| L1-Q32 | L1 | B_used | over capacity | n/a | W = 40849 > 32768 | paired Q32 against Q64 |
| L1-Q32 | L1 | B_heap | over capacity | n/a | W = 40849 > 32768 | paired Q32 against Q64 |
| L1-opt-Q32 | L1-opt | time | over capacity | n/a | W = 40849 > 32768 | paired Q32 against Q64 |
| L1-opt-Q32 | L1-opt | B_used | over capacity | n/a | W = 40849 > 32768 | paired Q32 against Q64 |
| L1-opt-Q32 | L1-opt | B_heap | over capacity | n/a | W = 40849 > 32768 | paired Q32 against Q64 |
| L1-Q32 | ctl | time | over capacity | n/a | W = 40849 > 32768 |  |
| L1-Q32 | ctl | B_used | over capacity | n/a | W = 40849 > 32768 |  |
| L1-Q32 | ctl | B_heap | over capacity | n/a | W = 40849 > 32768 |  |
| L1-Q32 | fmpq | time | over capacity | n/a | W = 40849 > 32768 |  |
| L1-Q32 | fmpq | B_used | over capacity | n/a | W = 40849 > 32768 |  |
| L1-Q32 | fmpq | B_heap | over capacity | n/a | W = 40849 > 32768 |  |
| L1-Q32 | mpq | time | over capacity | n/a | W = 40849 > 32768 |  |
| L1-Q32 | mpq | B_used | over capacity | n/a | W = 40849 > 32768 |  |
| L1-Q32 | mpq | B_heap | over capacity | n/a | W = 40849 > 32768 |  |
| L1-Q32 | best | time | over capacity | n/a | W = 40849 > 32768 |  |
| L1-opt-Q32 | ctl | time | over capacity | n/a | W = 40849 > 32768 |  |
| L1-opt-Q32 | ctl | B_used | over capacity | n/a | W = 40849 > 32768 |  |
| L1-opt-Q32 | ctl | B_heap | over capacity | n/a | W = 40849 > 32768 |  |
| L1-opt-Q32 | fmpq | time | over capacity | n/a | W = 40849 > 32768 |  |
| L1-opt-Q32 | fmpq | B_used | over capacity | n/a | W = 40849 > 32768 |  |
| L1-opt-Q32 | fmpq | B_heap | over capacity | n/a | W = 40849 > 32768 |  |
| L1-opt-Q32 | mpq | time | over capacity | n/a | W = 40849 > 32768 |  |
| L1-opt-Q32 | mpq | B_used | over capacity | n/a | W = 40849 > 32768 |  |
| L1-opt-Q32 | mpq | B_heap | over capacity | n/a | W = 40849 > 32768 |  |
| L1-opt-Q32 | best | time | over capacity | n/a | W = 40849 > 32768 |  |

Operation time alone (entry and read-out excluded; rule item 12, verdicts by item 8 with the job's h): L1-Q32 vs L1 over capacity n/a; L1-Q32 vs ctl over capacity n/a; L1-Q32 vs fmpq over capacity n/a; L1-Q32 vs mpq over capacity n/a; L1-opt-Q32 vs L1-opt over capacity n/a; L1-opt-Q32 vs ctl over capacity n/a; L1-opt-Q32 vs fmpq over capacity n/a; L1-opt-Q32 vs mpq over capacity n/a

#### Job 6 (MOD-muldiv)

Null control L1 against L1-null (this run): ratio 0.999 [0.993, 1.004], h = 0.050

L1-Q32 and L1-opt-Q32: **over capacity** (W = 40798 > 32,768); every Q32 verdict of this job is over capacity.

| X | Y | measure | verdict | ratio | interval / after entry | flags |
| --- | --- | --- | --- | --- | --- | --- |
| L1-Q32 | L1 | time | over capacity | n/a | W = 40798 > 32768 | paired Q32 against Q64 |
| L1-Q32 | L1 | B_used | over capacity | n/a | W = 40798 > 32768 | paired Q32 against Q64 |
| L1-Q32 | L1 | B_heap | over capacity | n/a | W = 40798 > 32768 | paired Q32 against Q64 |
| L1-opt-Q32 | L1-opt | time | over capacity | n/a | W = 40798 > 32768 | paired Q32 against Q64 |
| L1-opt-Q32 | L1-opt | B_used | over capacity | n/a | W = 40798 > 32768 | paired Q32 against Q64 |
| L1-opt-Q32 | L1-opt | B_heap | over capacity | n/a | W = 40798 > 32768 | paired Q32 against Q64 |
| L1-Q32 | ctl | time | over capacity | n/a | W = 40798 > 32768 |  |
| L1-Q32 | ctl | B_used | over capacity | n/a | W = 40798 > 32768 |  |
| L1-Q32 | ctl | B_heap | over capacity | n/a | W = 40798 > 32768 |  |
| L1-Q32 | fmpq | time | over capacity | n/a | W = 40798 > 32768 |  |
| L1-Q32 | fmpq | B_used | over capacity | n/a | W = 40798 > 32768 |  |
| L1-Q32 | fmpq | B_heap | over capacity | n/a | W = 40798 > 32768 |  |
| L1-Q32 | mpq | time | over capacity | n/a | W = 40798 > 32768 |  |
| L1-Q32 | mpq | B_used | over capacity | n/a | W = 40798 > 32768 |  |
| L1-Q32 | mpq | B_heap | over capacity | n/a | W = 40798 > 32768 |  |
| L1-Q32 | best | time | over capacity | n/a | W = 40798 > 32768 |  |
| L1-opt-Q32 | ctl | time | over capacity | n/a | W = 40798 > 32768 |  |
| L1-opt-Q32 | ctl | B_used | over capacity | n/a | W = 40798 > 32768 |  |
| L1-opt-Q32 | ctl | B_heap | over capacity | n/a | W = 40798 > 32768 |  |
| L1-opt-Q32 | fmpq | time | over capacity | n/a | W = 40798 > 32768 |  |
| L1-opt-Q32 | fmpq | B_used | over capacity | n/a | W = 40798 > 32768 |  |
| L1-opt-Q32 | fmpq | B_heap | over capacity | n/a | W = 40798 > 32768 |  |
| L1-opt-Q32 | mpq | time | over capacity | n/a | W = 40798 > 32768 |  |
| L1-opt-Q32 | mpq | B_used | over capacity | n/a | W = 40798 > 32768 |  |
| L1-opt-Q32 | mpq | B_heap | over capacity | n/a | W = 40798 > 32768 |  |
| L1-opt-Q32 | best | time | over capacity | n/a | W = 40798 > 32768 |  |

Operation time alone (entry and read-out excluded; rule item 12, verdicts by item 8 with the job's h): L1-Q32 vs L1 over capacity n/a; L1-Q32 vs ctl over capacity n/a; L1-Q32 vs fmpq over capacity n/a; L1-Q32 vs mpq over capacity n/a; L1-opt-Q32 vs L1-opt over capacity n/a; L1-opt-Q32 vs ctl over capacity n/a; L1-opt-Q32 vs fmpq over capacity n/a; L1-opt-Q32 vs mpq over capacity n/a

#### Job 7 (NEW-add)

Null control L1 against L1-null (this run): ratio 0.999 [0.996, 1.004], h = 0.050

L1-Q32 and L1-opt-Q32: **over capacity** (W = 50795 > 32,768); every Q32 verdict of this job is over capacity.

| X | Y | measure | verdict | ratio | interval / after entry | flags |
| --- | --- | --- | --- | --- | --- | --- |
| L1-Q32 | L1 | time | over capacity | n/a | W = 50795 > 32768 | paired Q32 against Q64 |
| L1-Q32 | L1 | B_used | over capacity | n/a | W = 50795 > 32768 | paired Q32 against Q64 |
| L1-Q32 | L1 | B_heap | over capacity | n/a | W = 50795 > 32768 | paired Q32 against Q64 |
| L1-opt-Q32 | L1-opt | time | over capacity | n/a | W = 50795 > 32768 | paired Q32 against Q64 |
| L1-opt-Q32 | L1-opt | B_used | over capacity | n/a | W = 50795 > 32768 | paired Q32 against Q64 |
| L1-opt-Q32 | L1-opt | B_heap | over capacity | n/a | W = 50795 > 32768 | paired Q32 against Q64 |
| L1-Q32 | ctl | time | over capacity | n/a | W = 50795 > 32768 |  |
| L1-Q32 | ctl | B_used | over capacity | n/a | W = 50795 > 32768 |  |
| L1-Q32 | ctl | B_heap | over capacity | n/a | W = 50795 > 32768 |  |
| L1-Q32 | fmpq | time | over capacity | n/a | W = 50795 > 32768 |  |
| L1-Q32 | fmpq | B_used | over capacity | n/a | W = 50795 > 32768 |  |
| L1-Q32 | fmpq | B_heap | over capacity | n/a | W = 50795 > 32768 |  |
| L1-Q32 | mpq | time | over capacity | n/a | W = 50795 > 32768 |  |
| L1-Q32 | mpq | B_used | over capacity | n/a | W = 50795 > 32768 |  |
| L1-Q32 | mpq | B_heap | over capacity | n/a | W = 50795 > 32768 |  |
| L1-Q32 | best | time | over capacity | n/a | W = 50795 > 32768 |  |
| L1-opt-Q32 | ctl | time | over capacity | n/a | W = 50795 > 32768 |  |
| L1-opt-Q32 | ctl | B_used | over capacity | n/a | W = 50795 > 32768 |  |
| L1-opt-Q32 | ctl | B_heap | over capacity | n/a | W = 50795 > 32768 |  |
| L1-opt-Q32 | fmpq | time | over capacity | n/a | W = 50795 > 32768 |  |
| L1-opt-Q32 | fmpq | B_used | over capacity | n/a | W = 50795 > 32768 |  |
| L1-opt-Q32 | fmpq | B_heap | over capacity | n/a | W = 50795 > 32768 |  |
| L1-opt-Q32 | mpq | time | over capacity | n/a | W = 50795 > 32768 |  |
| L1-opt-Q32 | mpq | B_used | over capacity | n/a | W = 50795 > 32768 |  |
| L1-opt-Q32 | mpq | B_heap | over capacity | n/a | W = 50795 > 32768 |  |
| L1-opt-Q32 | best | time | over capacity | n/a | W = 50795 > 32768 |  |

Operation time alone (entry and read-out excluded; rule item 12, verdicts by item 8 with the job's h): L1-Q32 vs L1 over capacity n/a; L1-Q32 vs ctl over capacity n/a; L1-Q32 vs fmpq over capacity n/a; L1-Q32 vs mpq over capacity n/a; L1-opt-Q32 vs L1-opt over capacity n/a; L1-opt-Q32 vs ctl over capacity n/a; L1-opt-Q32 vs fmpq over capacity n/a; L1-opt-Q32 vs mpq over capacity n/a

#### Job 8 (CH-money)

Null control L1 against L1-null (this run): ratio 1.000 [0.997, 1.002], h = 0.050

| X | Y | measure | verdict | ratio | interval / after entry | flags |
| --- | --- | --- | --- | --- | --- | --- |
| L1-Q32 | L1 | time | tie | 1.023 | [1.016, 1.026], h 0.050 | paired Q32 against Q64 |
| L1-Q32 | L1 | B_used | win | 0.636 | 0.510 | paired Q32 against Q64 |
| L1-Q32 | L1 | B_heap | win | 0.628 | 0.521 | paired Q32 against Q64 |
| L1-opt-Q32 | L1-opt | time | tie | 0.988 | [0.982, 0.990], h 0.050 | paired Q32 against Q64 |
| L1-opt-Q32 | L1-opt | B_used | win | 0.724 | 0.522 | paired Q32 against Q64 |
| L1-opt-Q32 | L1-opt | B_heap | win | 0.736 | 0.512 | paired Q32 against Q64 |
| L1-Q32 | ctl | time | loss | 0.706 | [0.701, 0.709], h 0.050 |  |
| L1-Q32 | ctl | B_used | win | 0.777 | 0.267 |  |
| L1-Q32 | ctl | B_heap | win | 0.632 | 0.281 |  |
| L1-Q32 | fmpq | time | loss | 0.747 | [0.742, 0.751], h 0.050 | chain: every result kept |
| L1-Q32 | fmpq | B_used | win | 0.777 | 0.267 | chain: every result kept |
| L1-Q32 | fmpq | B_heap | tie | 0.984 | 0.460 | chain: every result kept |
| L1-Q32 | mpq | time | win | 1.978 | [1.965, 2.005], h 0.050 | chain: every result kept |
| L1-Q32 | mpq | B_used | win | 0.229 | 0.089 | chain: every result kept |
| L1-Q32 | mpq | B_heap | win | 0.165 | 0.077 | chain: every result kept |
| L1-Q32 | fz | time | loss | 0.708 | [0.703, 0.710], h 0.050 | ceiling gap |
| L1-Q32 | fz | B_used | loss | 1.553 | 0.533 | ceiling gap |
| L1-Q32 | fz | B_heap | loss | 1.956 | 0.920 | ceiling gap |
| L1-Q32 | best | time | loss | 0.747 | [0.742, 0.751], h 0.050 | chain: every result kept, best library: fmpq |
| L1-opt-Q32 | ctl | time | loss | 0.679 | [0.676, 0.682], h 0.050 |  |
| L1-opt-Q32 | ctl | B_used | loss | 1.167 | 0.280 |  |
| L1-opt-Q32 | ctl | B_heap | win | 0.789 | 0.674 |  |
| L1-opt-Q32 | fmpq | time | loss | 0.719 | [0.716, 0.722], h 0.050 | chain: every result kept |
| L1-opt-Q32 | fmpq | B_used | loss | 1.167 | 0.279 | chain: every result kept |
| L1-opt-Q32 | fmpq | B_heap | loss | 1.229 | 1.105 | chain: every result kept |
| L1-opt-Q32 | mpq | time | win | 1.903 | [1.894, 1.928], h 0.050 | chain: every result kept |
| L1-opt-Q32 | mpq | B_used | win | 0.343 | 0.093 | chain: every result kept |
| L1-opt-Q32 | mpq | B_heap | win | 0.206 | 0.184 | chain: every result kept |
| L1-opt-Q32 | fz | time | loss | 0.681 | [0.677, 0.684], h 0.050 | ceiling gap |
| L1-opt-Q32 | fz | B_used | loss | 2.333 | 0.558 | severe memory loss, ceiling gap |
| L1-opt-Q32 | fz | B_heap | loss | 2.443 | 2.208 | severe memory loss, ceiling gap |
| L1-opt-Q32 | best | time | loss | 0.719 | [0.716, 0.722], h 0.050 | chain: every result kept, best library: fmpq |

Operation time alone (entry and read-out excluded; rule item 12, verdicts by item 8 with the job's h): L1-Q32 vs L1 tie 1.023 [1.017, 1.029]; L1-Q32 vs ctl loss 0.698 [0.693, 0.700]; L1-Q32 vs fmpq loss 0.716 [0.711, 0.720]; L1-Q32 vs mpq win 1.948 [1.934, 1.980]; L1-Q32 vs fz loss 0.090 [0.090, 0.092]; L1-opt-Q32 vs L1-opt tie 0.982 [0.978, 0.984]; L1-opt-Q32 vs ctl loss 0.668 [0.665, 0.670]; L1-opt-Q32 vs fmpq loss 0.686 [0.683, 0.688]; L1-opt-Q32 vs mpq win 1.865 [1.856, 1.897]; L1-opt-Q32 vs fz loss 0.086 [0.086, 0.088]

*Limitation:* on this chain job every result is kept as a new element, which is not the libraries' normal use (spec §10 item 3; rule, Limitations).

#### Job 9 (CH-closed)

Null control L1 against L1-null (this run): ratio 0.999 [0.995, 1.005], h = 0.050

| X | Y | measure | verdict | ratio | interval / after entry | flags |
| --- | --- | --- | --- | --- | --- | --- |
| L1-Q32 | L1 | time | tie | 1.030 | [1.026, 1.033], h 0.050 | paired Q32 against Q64 |
| L1-Q32 | L1 | B_used | win | 0.638 | 0.594 | paired Q32 against Q64 |
| L1-Q32 | L1 | B_heap | win | 0.629 | 0.597 | paired Q32 against Q64 |
| L1-opt-Q32 | L1-opt | time | tie | 0.985 | [0.983, 0.988], h 0.050 | paired Q32 against Q64 |
| L1-opt-Q32 | L1-opt | B_used | win | 0.725 | 0.672 | paired Q32 against Q64 |
| L1-opt-Q32 | L1-opt | B_heap | win | 0.736 | 0.580 | paired Q32 against Q64 |
| L1-Q32 | ctl | time | loss | 0.729 | [0.727, 0.731], h 0.050 |  |
| L1-Q32 | ctl | B_used | win | 0.783 | 0.516 |  |
| L1-Q32 | ctl | B_heap | win | 0.632 | 0.461 |  |
| L1-Q32 | fmpq | time | loss | 0.825 | [0.822, 0.827], h 0.050 | chain: every result kept |
| L1-Q32 | fmpq | B_used | win | 0.783 | 0.515 | chain: every result kept |
| L1-Q32 | fmpq | B_heap | tie | 0.984 | 0.755 | chain: every result kept |
| L1-Q32 | mpq | time | win | 1.870 | [1.862, 1.879], h 0.050 | chain: every result kept |
| L1-Q32 | mpq | B_used | win | 0.231 | 0.172 | chain: every result kept |
| L1-Q32 | mpq | B_heap | win | 0.165 | 0.126 | chain: every result kept |
| L1-Q32 | best | time | loss | 0.825 | [0.822, 0.827], h 0.050 | chain: every result kept, best library: fmpq |
| L1-opt-Q32 | ctl | time | loss | 0.711 | [0.710, 0.713], h 0.050 |  |
| L1-opt-Q32 | ctl | B_used | loss | 1.173 | 0.721 |  |
| L1-opt-Q32 | ctl | B_heap | win | 0.789 | 0.967 |  |
| L1-opt-Q32 | fmpq | time | loss | 0.805 | [0.803, 0.806], h 0.050 | chain: every result kept |
| L1-opt-Q32 | fmpq | B_used | loss | 1.173 | 0.720 | chain: every result kept |
| L1-opt-Q32 | fmpq | B_heap | loss | 1.229 | 1.585 | chain: every result kept |
| L1-opt-Q32 | mpq | time | win | 1.823 | [1.816, 1.832], h 0.050 | chain: every result kept |
| L1-opt-Q32 | mpq | B_used | win | 0.346 | 0.240 | chain: every result kept |
| L1-opt-Q32 | mpq | B_heap | win | 0.206 | 0.264 | chain: every result kept |
| L1-opt-Q32 | best | time | loss | 0.805 | [0.803, 0.806], h 0.050 | chain: every result kept, best library: fmpq |

Operation time alone (entry and read-out excluded; rule item 12, verdicts by item 8 with the job's h): L1-Q32 vs L1 tie 1.031 [1.026, 1.035]; L1-Q32 vs ctl loss 0.735 [0.733, 0.737]; L1-Q32 vs fmpq loss 0.820 [0.817, 0.822]; L1-Q32 vs mpq win 1.862 [1.856, 1.874]; L1-opt-Q32 vs L1-opt tie 0.985 [0.984, 0.990]; L1-opt-Q32 vs ctl loss 0.712 [0.709, 0.713]; L1-opt-Q32 vs fmpq loss 0.794 [0.793, 0.796]; L1-opt-Q32 vs mpq win 1.802 [1.799, 1.812]

*Limitation:* on this chain job every result is kept as a new element, which is not the libraries' normal use (spec §10 item 3; rule, Limitations).

#### Job 10 (CH-small)

Null control L1 against L1-null (this run): ratio 1.000 [0.995, 1.005], h = 0.050

| X | Y | measure | verdict | ratio | interval / after entry | flags |
| --- | --- | --- | --- | --- | --- | --- |
| L1-Q32 | L1 | time | tie | 1.017 | [1.011, 1.022], h 0.050 | paired Q32 against Q64 |
| L1-Q32 | L1 | B_used | win | 0.621 | 0.503 | paired Q32 against Q64 |
| L1-Q32 | L1 | B_heap | win | 0.629 | 0.500 | paired Q32 against Q64 |
| L1-opt-Q32 | L1-opt | time | tie | 0.976 | [0.971, 0.983], h 0.050 | paired Q32 against Q64 |
| L1-opt-Q32 | L1-opt | B_used | win | 0.721 | 0.506 | paired Q32 against Q64 |
| L1-opt-Q32 | L1-opt | B_heap | win | 0.748 | 0.507 | paired Q32 against Q64 |
| L1-Q32 | ctl | time | loss | 0.664 | [0.658, 0.667], h 0.050 |  |
| L1-Q32 | ctl | B_used | win | 0.782 | 0.255 |  |
| L1-Q32 | ctl | B_heap | win | 0.637 | 0.258 |  |
| L1-Q32 | fmpq | time | loss | 0.725 | [0.720, 0.734], h 0.050 | chain: every result kept |
| L1-Q32 | fmpq | B_used | win | 0.781 | 0.255 | chain: every result kept |
| L1-Q32 | fmpq | B_heap | loss | 1.152 | 0.422 | chain: every result kept |
| L1-Q32 | mpq | time | win | 1.589 | [1.578, 1.620], h 0.050 | chain: every result kept |
| L1-Q32 | mpq | B_used | win | 0.227 | 0.085 | chain: every result kept |
| L1-Q32 | mpq | B_heap | win | 0.194 | 0.070 | chain: every result kept |
| L1-Q32 | best | time | loss | 0.725 | [0.720, 0.734], h 0.050 | chain: every result kept, best library: fmpq |
| L1-opt-Q32 | ctl | time | loss | 0.633 | [0.629, 0.637], h 0.050 |  |
| L1-opt-Q32 | ctl | B_used | loss | 1.237 | 0.258 |  |
| L1-opt-Q32 | ctl | B_heap | win | 0.771 | 0.284 |  |
| L1-opt-Q32 | fmpq | time | loss | 0.692 | [0.687, 0.700], h 0.050 | chain: every result kept |
| L1-opt-Q32 | fmpq | B_used | loss | 1.236 | 0.258 | chain: every result kept |
| L1-opt-Q32 | fmpq | B_heap | loss | 1.395 | 0.466 | chain: every result kept |
| L1-opt-Q32 | mpq | time | win | 1.515 | [1.507, 1.546], h 0.050 | chain: every result kept |
| L1-opt-Q32 | mpq | B_used | win | 0.359 | 0.086 | chain: every result kept |
| L1-opt-Q32 | mpq | B_heap | win | 0.235 | 0.078 | chain: every result kept |
| L1-opt-Q32 | best | time | loss | 0.692 | [0.687, 0.700], h 0.050 | chain: every result kept, best library: fmpq |

Operation time alone (entry and read-out excluded; rule item 12, verdicts by item 8 with the job's h): L1-Q32 vs L1 tie 1.015 [1.009, 1.020]; L1-Q32 vs ctl loss 0.630 [0.626, 0.634]; L1-Q32 vs fmpq loss 0.671 [0.666, 0.677]; L1-Q32 vs mpq win 1.391 [1.375, 1.421]; L1-opt-Q32 vs L1-opt tie 0.976 [0.970, 0.982]; L1-opt-Q32 vs ctl loss 0.588 [0.585, 0.593]; L1-opt-Q32 vs fmpq loss 0.626 [0.621, 0.632]; L1-opt-Q32 vs mpq win 1.298 [1.286, 1.325]

*Limitation:* on this chain job every result is kept as a new element, which is not the libraries' normal use (spec §10 item 3; rule, Limitations).

#### Job 11 (CH-mixed)

Null control L1 against L1-null (this run): ratio 1.001 [0.999, 1.003], h = 0.050

| X | Y | measure | verdict | ratio | interval / after entry | flags |
| --- | --- | --- | --- | --- | --- | --- |
| L1-Q32 | L1 | time | tie | 1.028 | [1.025, 1.031], h 0.050 | paired Q32 against Q64 |
| L1-Q32 | L1 | B_used | win | 0.635 | 0.503 | paired Q32 against Q64 |
| L1-Q32 | L1 | B_heap | win | 0.629 | 0.500 | paired Q32 against Q64 |
| L1-opt-Q32 | L1-opt | time | tie | 0.978 | [0.971, 0.982], h 0.050 | paired Q32 against Q64 |
| L1-opt-Q32 | L1-opt | B_used | win | 0.729 | 0.506 | paired Q32 against Q64 |
| L1-opt-Q32 | L1-opt | B_heap | win | 0.748 | 0.507 | paired Q32 against Q64 |
| L1-Q32 | ctl | time | loss | 0.675 | [0.672, 0.678], h 0.050 |  |
| L1-Q32 | ctl | B_used | win | 0.830 | 0.255 |  |
| L1-Q32 | ctl | B_heap | win | 0.637 | 0.258 |  |
| L1-Q32 | fmpq | time | loss | 0.752 | [0.750, 0.754], h 0.050 | chain: every result kept |
| L1-Q32 | fmpq | B_used | win | 0.829 | 0.255 | chain: every result kept |
| L1-Q32 | fmpq | B_heap | loss | 1.152 | 0.422 | chain: every result kept |
| L1-Q32 | mpq | time | win | 1.569 | [1.562, 1.589], h 0.050 | chain: every result kept |
| L1-Q32 | mpq | B_used | win | 0.244 | 0.085 | chain: every result kept |
| L1-Q32 | mpq | B_heap | win | 0.194 | 0.070 | chain: every result kept |
| L1-Q32 | best | time | loss | 0.752 | [0.750, 0.754], h 0.050 | chain: every result kept, best library: fmpq |
| L1-opt-Q32 | ctl | time | loss | 0.650 | [0.646, 0.653], h 0.050 |  |
| L1-opt-Q32 | ctl | B_used | loss | 1.285 | 0.258 |  |
| L1-opt-Q32 | ctl | B_heap | win | 0.771 | 0.284 |  |
| L1-opt-Q32 | fmpq | time | loss | 0.725 | [0.719, 0.727], h 0.050 | chain: every result kept |
| L1-opt-Q32 | fmpq | B_used | loss | 1.284 | 0.258 | chain: every result kept |
| L1-opt-Q32 | fmpq | B_heap | loss | 1.395 | 0.466 | chain: every result kept |
| L1-opt-Q32 | mpq | time | win | 1.512 | [1.499, 1.533], h 0.050 | chain: every result kept |
| L1-opt-Q32 | mpq | B_used | win | 0.378 | 0.086 | chain: every result kept |
| L1-opt-Q32 | mpq | B_heap | win | 0.235 | 0.078 | chain: every result kept |
| L1-opt-Q32 | best | time | loss | 0.725 | [0.719, 0.727], h 0.050 | chain: every result kept, best library: fmpq |

Operation time alone (entry and read-out excluded; rule item 12, verdicts by item 8 with the job's h): L1-Q32 vs L1 tie 1.031 [1.028, 1.034]; L1-Q32 vs ctl loss 0.648 [0.645, 0.652]; L1-Q32 vs fmpq loss 0.707 [0.706, 0.710]; L1-Q32 vs mpq win 1.358 [1.350, 1.383]; L1-opt-Q32 vs L1-opt tie 0.980 [0.969, 0.984]; L1-opt-Q32 vs ctl loss 0.613 [0.607, 0.615]; L1-opt-Q32 vs fmpq loss 0.669 [0.662, 0.672]; L1-opt-Q32 vs mpq win 1.283 [1.269, 1.309]

*Limitation:* on this chain job every result is kept as a new element, which is not the libraries' normal use (spec §10 item 3; rule, Limitations).

## Gcd work against time, Q32 rows (rule item 12)

A job where the Q32 row removes gcd work or multiplications against ctl (verdict win by the fixed bands of item 8) but has no time win against ctl is a **candidate for a wider-number test**, where both cost more. The counts are the row's own counting run, equal to its Q64 row's (checked, spec §18.3 item 1); the time verdict is this run's. L1's and L1-opt's table is official at b3b88b6 ([archived results](../results-q64-b3b88b6/results.md)). The Q32 rows are not compared with each other (rule item 1, revision), so L1-opt-Q32 against L1-Q32 has no time verdict and no row here.

| job | X | Y | gcd work | multiplications | time | candidate |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | L1-Q32 | ctl | tie 1.000 | tie 1.000 | loss 0.698 | no |
| 1 | L1-opt-Q32 | ctl | tie 0.997 | tie 0.983 | loss 0.659 | no |
| 2 | L1-Q32 | ctl | tie 1.000 | tie 1.000 | loss 0.709 | no |
| 2 | L1-opt-Q32 | ctl | tie 0.995 | tie 0.981 | loss 0.681 | no |
| 3 | L1-Q32 | ctl | win 0.853 | win 0.869 | loss 0.890 | yes |
| 3 | L1-opt-Q32 | ctl | win 0.654 | win 0.576 | tie 0.954 | yes |
| 4 | L1-Q32 | ctl | tie 0.987 | tie 0.985 | loss 0.704 | no |
| 4 | L1-opt-Q32 | ctl | tie 0.958 | tie 0.947 | loss 0.688 | no |
| 5 | L1-Q32 | ctl | over capacity | over capacity | over capacity | - |
| 5 | L1-opt-Q32 | ctl | over capacity | over capacity | over capacity | - |
| 6 | L1-Q32 | ctl | over capacity | over capacity | over capacity | - |
| 6 | L1-opt-Q32 | ctl | over capacity | over capacity | over capacity | - |
| 7 | L1-Q32 | ctl | over capacity | over capacity | over capacity | - |
| 7 | L1-opt-Q32 | ctl | over capacity | over capacity | over capacity | - |
| 8 | L1-Q32 | ctl | tie 1.000 | tie 1.000 | loss 0.706 | no |
| 8 | L1-opt-Q32 | ctl | tie 0.994 | tie 0.979 | loss 0.679 | no |
| 9 | L1-Q32 | ctl | tie 1.000 | tie 1.000 | loss 0.729 | no |
| 9 | L1-opt-Q32 | ctl | tie 0.998 | tie 0.987 | loss 0.711 | no |
| 10 | L1-Q32 | ctl | tie 0.998 | tie 0.996 | loss 0.664 | no |
| 10 | L1-opt-Q32 | ctl | tie 0.976 | tie 0.966 | loss 0.633 | no |
| 11 | L1-Q32 | ctl | tie 0.999 | tie 0.997 | loss 0.675 | no |
| 11 | L1-opt-Q32 | ctl | tie 0.981 | tie 0.968 | loss 0.650 | no |

Candidates: job 3 (L1-Q32 against ctl); job 3 (L1-opt-Q32 against ctl).

## Q64 verdicts that changed between the two runs (rule, revision of Oct 29, 2026)

The official Q64 verdicts are those of commit b3b88b6. The Q64 rows were re-run here only for the paired comparison with the Q32 rows. Every (job, X, Y, measure) whose verdict differs between the two runs is listed, with both magnitudes; neither run is chosen over the other.

Official verdicts read from [results-q64-b3b88b6/verdicts.csv](../results-q64-b3b88b6/verdicts.csv) ([archived results](../results-q64-b3b88b6/results.md)).

0 changed, 348 unchanged.

## The model's bits per rational (rule, revision of Oct 29, 2026)

bits per rational = H + E W/N, E = 64 + b_s S/W (H the rational's field bits; b_s the bits per lookup slot: L1 32, L1-opt 64, L1-Q32 16, L1-opt-Q32 48), with W from the generator and S from the growth rule, made before any run. **For B_used this is an accounting identity**: it restates the memory report's sum, so the "identity" column checks the report to the byte and tests nothing about the design. **Only B_heap tests anything:** it is measured by the C library's heap statistics, with capacity slack and allocator headers that the model leaves out.

| job | when | row | W | N | S | S/W | E | predicted bits | B_used bits | identity | B_heap bits | B_heap / predicted |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | after entry | L1 | 9991 | 10000 | 32768 | 3.280 | 169.0 | 232.80 | 232.80 | ok | 319.53 | 1.373 |
| 1 | after entry | L1-opt | 9991 | 10000 | 32768 | 3.280 | 273.9 | 337.66 | 337.66 | ok | 509.06 | 1.508 |
| 1 | after entry | L1-Q32 | 9991 | 10000 | 32768 | 3.280 | 116.5 | 148.37 | 148.37 | ok | 214.77 | 1.448 |
| 1 | after entry | L1-opt-Q32 | 9991 | 10000 | 32768 | 3.280 | 221.4 | 253.23 | 253.23 | ok | 361.08 | 1.426 |
| 1 | end | L1 | 29866 | 30000 | 65536 | 2.194 | 134.2 | 197.62 | 197.62 | ok | 212.46 | 1.075 |
| 1 | end | L1-opt | 29866 | 30000 | 65536 | 2.194 | 204.4 | 267.52 | 267.52 | ok | 275.64 | 1.030 |
| 1 | end | L1-Q32 | 29866 | 30000 | 65536 | 2.194 | 99.1 | 130.67 | 130.67 | ok | 142.59 | 1.091 |
| 1 | end | L1-opt-Q32 | 29866 | 30000 | 65536 | 2.194 | 169.3 | 200.57 | 200.57 | ok | 208.84 | 1.041 |
| 2 | after entry | L1 | 209 | 10000 | 512 | 2.450 | 142.4 | 66.98 | 66.98 | ok | 112.26 | 1.676 |
| 2 | after entry | L1-opt | 209 | 10000 | 512 | 2.450 | 220.8 | 68.61 | 68.61 | ok | 199.40 | 2.906 |
| 2 | after entry | L1-Q32 | 209 | 10000 | 512 | 2.450 | 103.2 | 34.16 | 34.16 | ok | 58.69 | 1.718 |
| 2 | after entry | L1-opt-Q32 | 209 | 10000 | 512 | 2.450 | 181.6 | 35.80 | 35.80 | ok | 103.03 | 2.878 |
| 2 | end | L1 | 15403 | 30000 | 32768 | 2.127 | 132.1 | 131.81 | 131.81 | ok | 142.28 | 1.079 |
| 2 | end | L1-opt | 15403 | 30000 | 32768 | 2.127 | 200.2 | 166.76 | 166.76 | ok | 170.78 | 1.024 |
| 2 | end | L1-Q32 | 15403 | 30000 | 32768 | 2.127 | 98.0 | 82.34 | 82.34 | ok | 90.02 | 1.093 |
| 2 | end | L1-opt-Q32 | 15403 | 30000 | 32768 | 2.127 | 166.1 | 117.29 | 117.29 | ok | 121.45 | 1.036 |
| 3 | after entry | L1 | 8 | 10000 | 16 | 2.000 | 128.0 | 64.10 | 64.10 | ok | 105.14 | 1.640 |
| 3 | after entry | L1-opt | 8 | 10000 | 16 | 2.000 | 192.0 | 64.15 | 64.15 | ok | 192.35 | 2.998 |
| 3 | after entry | L1-Q32 | 8 | 10000 | 16 | 2.000 | 96.0 | 32.08 | 32.08 | ok | 52.68 | 1.642 |
| 3 | after entry | L1-opt-Q32 | 8 | 10000 | 16 | 2.000 | 160.0 | 32.13 | 32.13 | ok | 96.32 | 2.998 |
| 3 | end | L1 | 350 | 30000 | 1024 | 2.926 | 157.6 | 65.84 | 65.84 | ok | 74.31 | 1.129 |
| 3 | end | L1-opt | 350 | 30000 | 1024 | 2.926 | 251.2 | 66.93 | 66.93 | ok | 69.20 | 1.034 |
| 3 | end | L1-Q32 | 350 | 30000 | 1024 | 2.926 | 110.8 | 33.29 | 33.29 | ok | 38.80 | 1.165 |
| 3 | end | L1-opt-Q32 | 350 | 30000 | 1024 | 2.926 | 204.4 | 34.39 | 34.39 | ok | 36.80 | 1.070 |
| 4 | after entry | L1 | 64 | 10000 | 128 | 2.000 | 128.0 | 64.82 | 64.82 | ok | 108.16 | 1.669 |
| 4 | after entry | L1-opt | 64 | 10000 | 128 | 2.000 | 192.0 | 65.23 | 65.23 | ok | 194.46 | 2.981 |
| 4 | after entry | L1-Q32 | 64 | 10000 | 128 | 2.000 | 96.0 | 32.61 | 32.61 | ok | 54.08 | 1.658 |
| 4 | after entry | L1-opt-Q32 | 64 | 10000 | 128 | 2.000 | 160.0 | 33.02 | 33.02 | ok | 98.07 | 2.970 |
| 4 | end | L1 | 21346 | 30000 | 65536 | 3.070 | 162.2 | 179.44 | 179.44 | ok | 212.48 | 1.184 |
| 4 | end | L1-opt | 21346 | 30000 | 65536 | 3.070 | 260.5 | 249.35 | 249.35 | ok | 275.64 | 1.105 |
| 4 | end | L1-Q32 | 21346 | 30000 | 65536 | 3.070 | 113.1 | 112.49 | 112.49 | ok | 142.16 | 1.264 |
| 4 | end | L1-opt-Q32 | 21346 | 30000 | 65536 | 3.070 | 211.4 | 182.40 | 182.40 | ok | 208.84 | 1.145 |
| 5 | after entry | L1 | 1000 | 10000 | 2048 | 2.048 | 129.5 | 76.95 | 76.95 | ok | 122.70 | 1.594 |
| 5 | after entry | L1-opt | 1000 | 10000 | 2048 | 2.048 | 195.1 | 83.51 | 83.51 | ok | 214.14 | 2.564 |
| 5 | after entry | L1-Q32 | 1000 | 10000 | 2048 | 2.048 | 96.8 | 41.68 | over capacity | - | over capacity | - |
| 5 | after entry | L1-opt-Q32 | 1000 | 10000 | 2048 | 2.048 | 162.3 | 48.23 | over capacity | - | over capacity | - |
| 5 | end | L1 | 40849 | 30000 | 131072 | 3.209 | 166.7 | 290.95 | 290.95 | ok | 352.20 | 1.210 |
| 5 | end | L1-opt | 40849 | 30000 | 131072 | 3.209 | 269.4 | 430.76 | 430.76 | ok | 485.35 | 1.127 |
| 5 | end | L1-Q32 | 40849 | 30000 | 131072 | 3.209 | 115.3 | 189.05 | over capacity | - | over capacity | - |
| 5 | end | L1-opt-Q32 | 40849 | 30000 | 131072 | 3.209 | 218.0 | 328.86 | over capacity | - | over capacity | - |
| 6 | after entry | L1 | 1000 | 10000 | 2048 | 2.048 | 129.5 | 76.95 | 76.95 | ok | 122.92 | 1.597 |
| 6 | after entry | L1-opt | 1000 | 10000 | 2048 | 2.048 | 195.1 | 83.51 | 83.51 | ok | 214.14 | 2.564 |
| 6 | after entry | L1-Q32 | 1000 | 10000 | 2048 | 2.048 | 96.8 | 41.68 | over capacity | - | over capacity | - |
| 6 | after entry | L1-opt-Q32 | 1000 | 10000 | 2048 | 2.048 | 162.3 | 48.23 | over capacity | - | over capacity | - |
| 6 | end | L1 | 40798 | 30000 | 131072 | 3.213 | 166.8 | 290.85 | 290.85 | ok | 352.27 | 1.211 |
| 6 | end | L1-opt | 40798 | 30000 | 131072 | 3.213 | 269.6 | 430.66 | 430.66 | ok | 485.35 | 1.127 |
| 6 | end | L1-Q32 | 40798 | 30000 | 131072 | 3.213 | 115.4 | 188.94 | over capacity | - | over capacity | - |
| 6 | end | L1-opt-Q32 | 40798 | 30000 | 131072 | 3.213 | 218.2 | 328.75 | over capacity | - | over capacity | - |
| 7 | after entry | L1 | 10929 | 10000 | 32768 | 2.998 | 159.9 | 238.80 | 238.80 | ok | 318.69 | 1.335 |
| 7 | after entry | L1-opt | 10929 | 10000 | 32768 | 2.998 | 255.9 | 343.66 | 343.66 | ok | 509.06 | 1.481 |
| 7 | after entry | L1-Q32 | 10929 | 10000 | 32768 | 2.998 | 112.0 | 154.37 | over capacity | - | over capacity | - |
| 7 | after entry | L1-opt-Q32 | 10929 | 10000 | 32768 | 2.998 | 207.9 | 259.23 | over capacity | - | over capacity | - |
| 7 | end | L1 | 50795 | 30000 | 131072 | 2.580 | 146.6 | 312.17 | 312.17 | ok | 352.00 | 1.128 |
| 7 | end | L1-opt | 50795 | 30000 | 131072 | 2.580 | 229.1 | 451.98 | 451.98 | ok | 485.35 | 1.074 |
| 7 | end | L1-Q32 | 50795 | 30000 | 131072 | 2.580 | 105.3 | 210.27 | over capacity | - | over capacity | - |
| 7 | end | L1-opt-Q32 | 50795 | 30000 | 131072 | 2.580 | 187.9 | 350.08 | over capacity | - | over capacity | - |
| 8 | after entry | L1 | 208 | 10000 | 512 | 2.462 | 142.8 | 66.97 | 66.97 | ok | 113.13 | 1.689 |
| 8 | after entry | L1-opt | 208 | 10000 | 512 | 2.462 | 221.5 | 68.61 | 68.61 | ok | 276.20 | 4.026 |
| 8 | after entry | L1-Q32 | 208 | 10000 | 512 | 2.462 | 103.4 | 34.15 | 34.15 | ok | 58.89 | 1.725 |
| 8 | after entry | L1-opt-Q32 | 208 | 10000 | 512 | 2.462 | 182.2 | 35.79 | 35.79 | ok | 141.43 | 3.952 |
| 8 | end | L1 | 27881 | 42000 | 65536 | 2.351 | 139.2 | 156.42 | 156.42 | ok | 201.70 | 1.290 |
| 8 | end | L1-opt | 27881 | 42000 | 65536 | 2.351 | 214.4 | 206.35 | 206.35 | ok | 215.17 | 1.043 |
| 8 | end | L1-Q32 | 27881 | 42000 | 65536 | 2.351 | 101.6 | 99.45 | 99.45 | ok | 126.77 | 1.275 |
| 8 | end | L1-opt-Q32 | 27881 | 42000 | 65536 | 2.351 | 176.8 | 149.38 | 149.38 | ok | 158.31 | 1.060 |
| 9 | after entry | L1 | 3268 | 10000 | 8192 | 2.507 | 144.2 | 111.13 | 111.13 | ok | 162.02 | 1.458 |
| 9 | after entry | L1-opt | 3268 | 10000 | 8192 | 2.507 | 224.4 | 137.34 | 137.34 | ok | 349.93 | 2.548 |
| 9 | after entry | L1-Q32 | 3268 | 10000 | 8192 | 2.507 | 104.1 | 66.02 | 66.02 | ok | 96.69 | 1.465 |
| 9 | after entry | L1-opt-Q32 | 3268 | 10000 | 8192 | 2.507 | 184.3 | 92.24 | 92.24 | ok | 202.87 | 2.199 |
| 9 | end | L1 | 28379 | 42000 | 65536 | 2.309 | 137.9 | 157.18 | 157.18 | ok | 201.64 | 1.283 |
| 9 | end | L1-opt | 28379 | 42000 | 65536 | 2.309 | 211.8 | 207.11 | 207.11 | ok | 215.17 | 1.039 |
| 9 | end | L1-Q32 | 28379 | 42000 | 65536 | 2.309 | 100.9 | 100.21 | 100.21 | ok | 126.79 | 1.265 |
| 9 | end | L1-opt-Q32 | 28379 | 42000 | 65536 | 2.309 | 174.8 | 150.14 | 150.14 | ok | 158.31 | 1.054 |
| 10 | after entry | L1 | 63 | 10000 | 128 | 2.032 | 129.0 | 64.81 | 64.81 | ok | 108.16 | 1.669 |
| 10 | after entry | L1-opt | 63 | 10000 | 128 | 2.032 | 194.0 | 65.22 | 65.22 | ok | 117.66 | 1.804 |
| 10 | after entry | L1-Q32 | 63 | 10000 | 128 | 2.032 | 96.5 | 32.61 | 32.61 | ok | 54.08 | 1.658 |
| 10 | after entry | L1-opt-Q32 | 63 | 10000 | 128 | 2.032 | 161.5 | 33.02 | 33.02 | ok | 59.67 | 1.807 |
| 10 | end | L1 | 10945 | 18000 | 32768 | 2.994 | 159.8 | 161.17 | 161.17 | ok | 237.62 | 1.474 |
| 10 | end | L1-opt | 10945 | 18000 | 32768 | 2.994 | 255.6 | 219.42 | 219.42 | ok | 241.97 | 1.103 |
| 10 | end | L1-Q32 | 10945 | 18000 | 32768 | 2.994 | 111.9 | 100.04 | 100.04 | ok | 149.55 | 1.495 |
| 10 | end | L1-opt-Q32 | 10945 | 18000 | 32768 | 2.994 | 207.7 | 158.30 | 158.30 | ok | 181.09 | 1.144 |
| 11 | after entry | L1 | 63 | 10000 | 128 | 2.032 | 129.0 | 64.81 | 64.81 | ok | 108.16 | 1.669 |
| 11 | after entry | L1-opt | 63 | 10000 | 128 | 2.032 | 194.0 | 65.22 | 65.22 | ok | 117.66 | 1.804 |
| 11 | after entry | L1-Q32 | 63 | 10000 | 128 | 2.032 | 96.5 | 32.61 | 32.61 | ok | 54.08 | 1.658 |
| 11 | after entry | L1-opt-Q32 | 63 | 10000 | 128 | 2.032 | 161.5 | 33.02 | 33.02 | ok | 59.67 | 1.807 |
| 11 | end | L1 | 12682 | 18000 | 32768 | 2.584 | 146.7 | 167.35 | 167.35 | ok | 237.62 | 1.420 |
| 11 | end | L1-opt | 12682 | 18000 | 32768 | 2.584 | 229.4 | 225.60 | 225.60 | ok | 241.97 | 1.073 |
| 11 | end | L1-Q32 | 12682 | 18000 | 32768 | 2.584 | 105.3 | 106.22 | 106.22 | ok | 149.55 | 1.408 |
| 11 | end | L1-opt-Q32 | 12682 | 18000 | 32768 | 2.584 | 188.0 | 164.47 | 164.47 | ok | 181.09 | 1.101 |

## Also reported (rule item 12)

The time-dependent items of rule item 12 for L1 and L1-opt (gcd work against time) are official at commit b3b88b6 ([archived results](../results-q64-b3b88b6/results.md)); the Q32 rows' are above. The counts below are deterministic and equal to that run's.

### Memory against reuse (jobs by W/N at the end; bytes per stored rational, end)

| job | W/N | L1 used / heap | L1-opt used / heap | L1-Q32 used / heap | L1-opt-Q32 used / heap | ctl used / heap | fmpq used / heap | mpq used / heap | fz used / heap | L1 formula check |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 3 | 0.012 | 8.2 / 9.3 | 8.4 / 8.7 | 4.2 / 4.8 | 4.3 / 4.6 | 16.0 / 17.6 | 16.0 / 16.1 | 58.7 / 96.1 | - | ok |
| 2 | 0.513 | 16.5 / 17.8 | 20.8 / 21.3 | 10.3 / 11.3 | 14.7 / 15.2 | 16.0 / 17.6 | 16.0 / 16.1 | 53.5 / 96.1 | 8.0 / 8.1 | ok |
| 10 | 0.608 | 20.1 / 29.7 | 27.4 / 30.2 | 12.5 / 18.7 | 19.8 / 22.6 | 16.0 / 29.4 | 16.0 / 16.2 | 55.1 / 96.2 | - | ok |
| 8 | 0.664 | 19.6 / 25.2 | 25.8 / 26.9 | 12.4 / 15.8 | 18.7 / 19.8 | 16.0 / 25.1 | 16.0 / 16.1 | 54.4 / 96.1 | 8.0 / 8.1 | ok |
| 9 | 0.676 | 19.6 / 25.2 | 25.9 / 26.9 | 12.5 / 15.8 | 18.8 / 19.8 | 16.0 / 25.1 | 16.0 / 16.1 | 54.2 / 96.1 | - | ok |
| 11 | 0.705 | 20.9 / 29.7 | 28.2 / 30.2 | 13.3 / 18.7 | 20.6 / 22.6 | 16.0 / 29.4 | 16.0 / 16.2 | 54.3 / 96.2 | - | ok |
| 4 | 0.712 | 22.4 / 26.6 | 31.2 / 34.5 | 14.1 / 17.8 | 22.8 / 26.1 | 16.0 / 17.6 | 16.0 / 16.1 | 58.7 / 96.1 | - | ok |
| 1 | 0.996 | 24.7 / 26.6 | 33.4 / 34.5 | 16.3 / 17.8 | 25.1 / 26.1 | 16.0 / 17.6 | 16.0 / 16.1 | 53.5 / 96.1 | 8.0 / 8.1 | ok |
| 6 | 1.360 | 36.4 / 44.0 | 53.8 / 60.7 | over capacity | over capacity | 16.0 / 17.6 | 16.0 / 16.1 | 58.7 / 96.1 | - | ok |
| 5 | 1.362 | 36.4 / 44.0 | 53.8 / 60.7 | over capacity | over capacity | 16.0 / 17.6 | 16.0 / 16.1 | 57.5 / 96.1 | - | ok |
| 7 | 1.693 | 39.0 / 44.0 | 56.5 / 60.7 | over capacity | over capacity | 16.0 / 17.6 | 16.0 / 16.1 | 56.0 / 96.1 | - | ok |

### Per case (rule items 6, 12): operations, and gcds, gcd work and multiplications against ctl

Each operation's core counts attributed to its predicted case (spec §14.1 item 3); add and subtract together. The `den` and `general` rows of L1 against ctl are 1 exactly. No verdict.

| job | case | n | L1 gcds / ctl | ratio | L1 gcd work / ctl | ratio | L1 mul / ctl | ratio | L1-opt gcds / L1 | L1-opt gcd work / L1 | L1-opt mul / L1 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | add_den | 4958 | 4958 / 4958 | 1.000 | 109760 / 109760 | 1.000 | 0 / 0 | 0 against 0 | 4956 / 4958 (1.000) | 109728 / 109760 (1.000) | 0 / 0 (0 against 0) |
| 1 | add_general | 15042 | 29408 / 29408 | 1.000 | 419076 / 419076 | 1.000 | 45126 / 45126 | 1.000 | 29019 / 29408 (0.987) | 416839 / 419076 (0.995) | 44348 / 45126 (0.983) |
| 2 | add_den | 5021 | 5021 / 5021 | 1.000 | 77357 / 77357 | 1.000 | 0 / 0 | 0 against 0 | 4996 / 5021 (0.995) | 77201 / 77357 (0.998) | 0 / 0 (0 against 0) |
| 2 | add_general | 14979 | 29204 / 29204 | 1.000 | 320603 / 320603 | 1.000 | 44937 / 44937 | 1.000 | 28768 / 29204 (0.985) | 318072 / 320603 (0.992) | 44064 / 44937 (0.981) |
| 3 | mul_both | 182 | 0 / 352 | 0.000 | 0 / 3840 | 0.000 | 0 / 364 | 0.000 | 0 / 0 (0 against 0) | 0 / 0 (0 against 0) | 0 / 0 (0 against 0) |
| 3 | mul_den | 1075 | 0 / 0 | 0 against 0 | 0 / 0 | 0 against 0 | 2150 / 2150 | 1.000 | 0 / 0 (0 against 0) | 0 / 0 (0 against 0) | 1418 / 2150 (0.660) |
| 3 | mul_num | 858 | 0 / 1716 | 0.000 | 0 / 23888 | 0.000 | 1716 / 1716 | 1.000 | 0 / 0 (0 against 0) | 0 / 0 (0 against 0) | 1077 / 1716 (0.628) |
| 3 | mul_cross_ad | 837 | 837 / 1674 | 0.500 | 13449 / 21460 | 0.627 | 0 / 1674 | 0.000 | 612 / 837 (0.731) | 10249 / 13449 (0.762) | 0 / 0 (0 against 0) |
| 3 | mul_cross_cb | 764 | 764 / 1528 | 0.500 | 12056 / 19327 | 0.624 | 0 / 1528 | 0.000 | 564 / 764 (0.738) | 9341 / 12056 (0.775) | 0 / 0 (0 against 0) |
| 3 | mul_general | 6186 | 12372 / 12372 | 1.000 | 216674 / 216674 | 1.000 | 12372 / 12372 | 1.000 | 8231 / 12372 (0.665) | 151050 / 216674 (0.697) | 8231 / 12372 (0.665) |
| 3 | div_both | 195 | 0 / 195 | 0.000 | 0 / 2002 | 0.000 | 0 / 0 | 0 against 0 | 0 / 0 (0 against 0) | 0 / 0 (0 against 0) | 0 / 0 (0 against 0) |
| 3 | div_cross_ad | 1088 | 0 / 2129 | 0.000 | 0 / 30275 | 0.000 | 2176 / 2082 | 1.045 | 0 / 0 (0 against 0) | 0 / 0 (0 against 0) | 1483 / 2176 (0.682) |
| 3 | div_cross_cb | 881 | 0 / 1728 | 0.000 | 0 / 24850 | 0.000 | 1762 / 1694 | 1.040 | 0 / 0 (0 against 0) | 0 / 0 (0 against 0) | 1164 / 1762 (0.661) |
| 3 | div_num | 784 | 784 / 1568 | 0.500 | 12457 / 19988 | 0.623 | 0 / 1568 | 0.000 | 561 / 784 (0.716) | 9212 / 12457 (0.740) | 0 / 0 (0 against 0) |
| 3 | div_den | 802 | 802 / 802 | 1.000 | 12719 / 12719 | 1.000 | 0 / 0 | 0 against 0 | 580 / 802 (0.723) | 9562 / 12719 (0.752) | 0 / 0 (0 against 0) |
| 3 | div_general | 6348 | 12696 / 12696 | 1.000 | 223918 / 223918 | 1.000 | 12696 / 12696 | 1.000 | 8421 / 12696 (0.663) | 156255 / 223918 (0.698) | 8421 / 12696 (0.663) |
| 4 | mul_both | 2 | 0 / 4 | 0.000 | 0 / 59 | 0.000 | 0 / 4 | 0.000 | 0 / 0 (0 against 0) | 0 / 0 (0 against 0) | 0 / 0 (0 against 0) |
| 4 | mul_den | 105 | 0 / 0 | 0 against 0 | 0 / 0 | 0 against 0 | 210 / 210 | 1.000 | 0 / 0 (0 against 0) | 0 / 0 (0 against 0) | 202 / 210 (0.962) |
| 4 | mul_num | 102 | 0 / 204 | 0.000 | 0 / 3057 | 0.000 | 204 / 204 | 1.000 | 0 / 0 (0 against 0) | 0 / 0 (0 against 0) | 191 / 204 (0.936) |
| 4 | mul_cross_ad | 85 | 85 / 170 | 0.500 | 1297 / 2479 | 0.523 | 0 / 170 | 0.000 | 79 / 85 (0.929) | 1212 / 1297 (0.934) | 0 / 0 (0 against 0) |
| 4 | mul_cross_cb | 101 | 101 / 202 | 0.500 | 1589 / 3005 | 0.529 | 0 / 202 | 0.000 | 98 / 101 (0.970) | 1547 / 1589 (0.974) | 0 / 0 (0 against 0) |
| 4 | mul_general | 9607 | 19214 / 19214 | 1.000 | 401752 / 401752 | 1.000 | 19214 / 19214 | 1.000 | 18460 / 19214 (0.961) | 387550 / 401752 (0.965) | 18460 / 19214 (0.961) |
| 4 | div_both | 3 | 0 / 3 | 0.000 | 0 / 41 | 0.000 | 0 / 0 | 0 against 0 | 0 / 0 (0 against 0) | 0 / 0 (0 against 0) | 0 / 0 (0 against 0) |
| 4 | div_cross_ad | 75 | 0 / 150 | 0.000 | 0 / 2295 | 0.000 | 150 / 150 | 1.000 | 0 / 0 (0 against 0) | 0 / 0 (0 against 0) | 142 / 150 (0.947) |
| 4 | div_cross_cb | 107 | 0 / 214 | 0.000 | 0 / 3267 | 0.000 | 214 / 214 | 1.000 | 0 / 0 (0 against 0) | 0 / 0 (0 against 0) | 207 / 214 (0.967) |
| 4 | div_num | 102 | 102 / 204 | 0.500 | 1568 / 3022 | 0.519 | 0 / 204 | 0.000 | 97 / 102 (0.951) | 1496 / 1568 (0.954) | 0 / 0 (0 against 0) |
| 4 | div_den | 86 | 86 / 86 | 1.000 | 1270 / 1270 | 1.000 | 0 / 0 | 0 against 0 | 83 / 86 (0.965) | 1229 / 1270 (0.968) | 0 / 0 (0 against 0) |
| 4 | div_general | 9625 | 19250 / 19250 | 1.000 | 401188 / 401188 | 1.000 | 19250 / 19250 | 1.000 | 18501 / 19250 (0.961) | 387463 / 401188 (0.966) | 18501 / 19250 (0.961) |
| 5 | add_cross | 6 | 0 / 6 | 0.000 | 0 / 119 | 0.000 | 18 / 18 | 1.000 | 0 / 0 (0 against 0) | 0 / 0 (0 against 0) | 18 / 18 (1.000) |
| 5 | add_den | 6 | 6 / 6 | 1.000 | 122 / 122 | 1.000 | 0 / 0 | 0 against 0 | 6 / 6 (1.000) | 122 / 122 (1.000) | 0 / 0 (0 against 0) |
| 5 | add_num | 4 | 7 / 7 | 1.000 | 126 / 184 | 0.685 | 8 / 12 | 0.667 | 7 / 7 (1.000) | 126 / 126 (1.000) | 8 / 8 (1.000) |
| 5 | add_general | 9978 | 14251 / 14251 | 1.000 | 475991 / 475991 | 1.000 | 29934 / 29934 | 1.000 | 14251 / 14251 (1.000) | 475991 / 475991 (1.000) | 29934 / 29934 (1.000) |
| 5 | mul_den | 4 | 0 / 0 | 0 against 0 | 0 / 0 | 0 against 0 | 8 / 8 | 1.000 | 0 / 0 (0 against 0) | 0 / 0 (0 against 0) | 8 / 8 (1.000) |
| 5 | mul_num | 1 | 0 / 2 | 0.000 | 0 / 40 | 0.000 | 2 / 2 | 1.000 | 0 / 0 (0 against 0) | 0 / 0 (0 against 0) | 2 / 2 (1.000) |
| 5 | mul_cross_ad | 4 | 4 / 8 | 0.500 | 78 / 156 | 0.500 | 0 / 8 | 0.000 | 4 / 4 (1.000) | 78 / 78 (1.000) | 0 / 0 (0 against 0) |
| 5 | mul_cross_cb | 4 | 4 / 8 | 0.500 | 79 / 157 | 0.503 | 0 / 8 | 0.000 | 4 / 4 (1.000) | 79 / 79 (1.000) | 0 / 0 (0 against 0) |
| 5 | mul_general | 5090 | 10180 / 10180 | 1.000 | 285626 / 285626 | 1.000 | 10180 / 10180 | 1.000 | 10180 / 10180 (1.000) | 285626 / 285626 (1.000) | 10180 / 10180 (1.000) |
| 5 | div_cross_ad | 2 | 0 / 4 | 0.000 | 0 / 80 | 0.000 | 4 / 4 | 1.000 | 0 / 0 (0 against 0) | 0 / 0 (0 against 0) | 4 / 4 (1.000) |
| 5 | div_cross_cb | 2 | 0 / 4 | 0.000 | 0 / 80 | 0.000 | 4 / 4 | 1.000 | 0 / 0 (0 against 0) | 0 / 0 (0 against 0) | 4 / 4 (1.000) |
| 5 | div_den | 2 | 2 / 2 | 1.000 | 39 / 39 | 1.000 | 0 / 0 | 0 against 0 | 2 / 2 (1.000) | 39 / 39 (1.000) | 0 / 0 (0 against 0) |
| 5 | div_general | 4897 | 9794 / 9794 | 1.000 | 277042 / 277042 | 1.000 | 9794 / 9794 | 1.000 | 9794 / 9794 (1.000) | 277042 / 277042 (1.000) | 9794 / 9794 (1.000) |
| 6 | mul_den | 3 | 0 / 0 | 0 against 0 | 0 / 0 | 0 against 0 | 6 / 6 | 1.000 | 0 / 0 (0 against 0) | 0 / 0 (0 against 0) | 6 / 6 (1.000) |
| 6 | mul_num | 5 | 0 / 10 | 0.000 | 0 / 199 | 0.000 | 10 / 10 | 1.000 | 0 / 0 (0 against 0) | 0 / 0 (0 against 0) | 10 / 10 (1.000) |
| 6 | mul_cross_ad | 3 | 3 / 6 | 0.500 | 59 / 118 | 0.500 | 0 / 6 | 0.000 | 3 / 3 (1.000) | 59 / 59 (1.000) | 0 / 0 (0 against 0) |
| 6 | mul_cross_cb | 5 | 5 / 10 | 0.500 | 97 / 194 | 0.500 | 0 / 10 | 0.000 | 5 / 5 (1.000) | 97 / 97 (1.000) | 0 / 0 (0 against 0) |
| 6 | mul_general | 9971 | 19942 / 19942 | 1.000 | 557282 / 557282 | 1.000 | 19942 / 19942 | 1.000 | 19942 / 19942 (1.000) | 557282 / 557282 (1.000) | 19942 / 19942 (1.000) |
| 6 | div_cross_ad | 6 | 0 / 12 | 0.000 | 0 / 237 | 0.000 | 12 / 12 | 1.000 | 0 / 0 (0 against 0) | 0 / 0 (0 against 0) | 12 / 12 (1.000) |
| 6 | div_cross_cb | 2 | 0 / 4 | 0.000 | 0 / 79 | 0.000 | 4 / 4 | 1.000 | 0 / 0 (0 against 0) | 0 / 0 (0 against 0) | 4 / 4 (1.000) |
| 6 | div_num | 8 | 8 / 16 | 0.500 | 158 / 316 | 0.500 | 0 / 16 | 0.000 | 8 / 8 (1.000) | 158 / 158 (1.000) | 0 / 0 (0 against 0) |
| 6 | div_den | 9 | 9 / 9 | 1.000 | 179 / 179 | 1.000 | 0 / 0 | 0 against 0 | 9 / 9 (1.000) | 179 / 179 (1.000) | 0 / 0 (0 against 0) |
| 6 | div_general | 9988 | 19976 / 19976 | 1.000 | 555940 / 555940 | 1.000 | 19976 / 19976 | 1.000 | 19976 / 19976 (1.000) | 555940 / 555940 (1.000) | 19976 / 19976 (1.000) |
| 7 | add_den | 5 | 5 / 5 | 1.000 | 103 / 103 | 1.000 | 0 / 0 | 0 against 0 | 5 / 5 (1.000) | 103 / 103 (1.000) | 0 / 0 (0 against 0) |
| 7 | add_general | 19995 | 30020 / 30020 | 1.000 | 1026421 / 1026421 | 1.000 | 59985 / 59985 | 1.000 | 30020 / 30020 (1.000) | 1026421 / 1026421 (1.000) | 59985 / 59985 (1.000) |
| 8 | add_den | 7679 | 7679 / 7679 | 1.000 | 128964 / 128964 | 1.000 | 0 / 0 | 0 against 0 | 7663 / 7679 (0.998) | 128823 / 128964 (0.999) | 0 / 0 (0 against 0) |
| 8 | add_general | 24321 | 47224 / 47224 | 1.000 | 547868 / 547868 | 1.000 | 72963 / 72963 | 1.000 | 46447 / 47224 (0.984) | 543373 / 547868 (0.992) | 71409 / 72963 (0.979) |
| 9 | add_cross | 3 | 0 / 3 | 0.000 | 0 / 21 | 0.000 | 9 / 9 | 1.000 | 0 / 0 (0 against 0) | 0 / 0 (0 against 0) | 9 / 9 (1.000) |
| 9 | add_den | 156 | 156 / 156 | 1.000 | 2692 / 2692 | 1.000 | 0 / 0 | 0 against 0 | 156 / 156 (1.000) | 2692 / 2692 (1.000) | 0 / 0 (0 against 0) |
| 9 | add_num | 3 | 4 / 4 | 1.000 | 28 / 32 | 0.875 | 6 / 9 | 0.667 | 4 / 4 (1.000) | 28 / 28 (1.000) | 4 / 6 (0.667) |
| 9 | add_general | 31838 | 63269 / 63269 | 1.000 | 1103990 / 1103990 | 1.000 | 95514 / 95514 | 1.000 | 63138 / 63269 (0.998) | 1101852 / 1103990 (0.998) | 94274 / 95514 (0.987) |
| 10 | mul_den | 11 | 0 / 0 | 0 against 0 | 0 / 0 | 0 against 0 | 22 / 22 | 1.000 | 0 / 0 (0 against 0) | 0 / 0 (0 against 0) | 22 / 22 (1.000) |
| 10 | mul_num | 9 | 0 / 18 | 0.000 | 0 / 124 | 0.000 | 18 / 18 | 1.000 | 0 / 0 (0 against 0) | 0 / 0 (0 against 0) | 17 / 18 (0.944) |
| 10 | mul_cross_ad | 10 | 10 / 20 | 0.500 | 68 / 111 | 0.613 | 0 / 20 | 0.000 | 10 / 10 (1.000) | 68 / 68 (1.000) | 0 / 0 (0 against 0) |
| 10 | mul_cross_cb | 10 | 10 / 20 | 0.500 | 61 / 114 | 0.535 | 0 / 20 | 0.000 | 10 / 10 (1.000) | 61 / 61 (1.000) | 0 / 0 (0 against 0) |
| 10 | mul_general | 3978 | 7956 / 7956 | 1.000 | 122049 / 122049 | 1.000 | 7956 / 7956 | 1.000 | 7723 / 7956 (0.971) | 118873 / 122049 (0.974) | 7723 / 7956 (0.971) |
| 10 | div_cross_ad | 17 | 0 / 34 | 0.000 | 0 / 202 | 0.000 | 34 / 34 | 1.000 | 0 / 0 (0 against 0) | 0 / 0 (0 against 0) | 33 / 34 (0.971) |
| 10 | div_cross_cb | 7 | 0 / 14 | 0.000 | 0 / 81 | 0.000 | 14 / 14 | 1.000 | 0 / 0 (0 against 0) | 0 / 0 (0 against 0) | 14 / 14 (1.000) |
| 10 | div_num | 10 | 10 / 20 | 0.500 | 82 / 129 | 0.636 | 0 / 20 | 0.000 | 10 / 10 (1.000) | 82 / 82 (1.000) | 0 / 0 (0 against 0) |
| 10 | div_den | 14 | 14 / 14 | 1.000 | 91 / 91 | 1.000 | 0 / 0 | 0 against 0 | 13 / 14 (0.929) | 88 / 91 (0.967) | 0 / 0 (0 against 0) |
| 10 | div_general | 3934 | 7868 / 7868 | 1.000 | 121336 / 121336 | 1.000 | 7868 / 7868 | 1.000 | 7620 / 7868 (0.968) | 117859 / 121336 (0.971) | 7620 / 7868 (0.968) |
| 11 | add_cross | 12 | 0 / 12 | 0.000 | 0 / 71 | 0.000 | 36 / 36 | 1.000 | 0 / 0 (0 against 0) | 0 / 0 (0 against 0) | 30 / 36 (0.833) |
| 11 | add_den | 11 | 11 / 11 | 1.000 | 63 / 63 | 1.000 | 0 / 0 | 0 against 0 | 10 / 11 (0.909) | 60 / 63 (0.952) | 0 / 0 (0 against 0) |
| 11 | add_num | 10 | 13 / 13 | 1.000 | 73 / 88 | 0.830 | 20 / 30 | 0.667 | 13 / 13 (1.000) | 73 / 73 (1.000) | 20 / 20 (1.000) |
| 11 | add_general | 3967 | 5696 / 5696 | 1.000 | 106253 / 106253 | 1.000 | 11901 / 11901 | 1.000 | 5584 / 5696 (0.980) | 104489 / 106253 (0.983) | 11550 / 11901 (0.971) |
| 11 | mul_both | 2 | 0 / 4 | 0.000 | 0 / 20 | 0.000 | 0 / 4 | 0.000 | 0 / 0 (0 against 0) | 0 / 0 (0 against 0) | 0 / 0 (0 against 0) |
| 11 | mul_den | 8 | 0 / 0 | 0 against 0 | 0 / 0 | 0 against 0 | 16 / 16 | 1.000 | 0 / 0 (0 against 0) | 0 / 0 (0 against 0) | 16 / 16 (1.000) |
| 11 | mul_num | 7 | 0 / 14 | 0.000 | 0 / 77 | 0.000 | 14 / 14 | 1.000 | 0 / 0 (0 against 0) | 0 / 0 (0 against 0) | 14 / 14 (1.000) |
| 11 | mul_cross_ad | 11 | 11 / 22 | 0.500 | 60 / 117 | 0.513 | 0 / 22 | 0.000 | 11 / 11 (1.000) | 60 / 60 (1.000) | 0 / 0 (0 against 0) |
| 11 | mul_cross_cb | 5 | 5 / 10 | 0.500 | 35 / 55 | 0.636 | 0 / 10 | 0.000 | 5 / 5 (1.000) | 35 / 35 (1.000) | 0 / 0 (0 against 0) |
| 11 | mul_general | 2078 | 4156 / 4156 | 1.000 | 72197 / 72197 | 1.000 | 4156 / 4156 | 1.000 | 4052 / 4156 (0.975) | 70535 / 72197 (0.977) | 4052 / 4156 (0.975) |
| 11 | div_cross_ad | 4 | 0 / 8 | 0.000 | 0 / 45 | 0.000 | 8 / 8 | 1.000 | 0 / 0 (0 against 0) | 0 / 0 (0 against 0) | 8 / 8 (1.000) |
| 11 | div_cross_cb | 4 | 0 / 8 | 0.000 | 0 / 51 | 0.000 | 8 / 8 | 1.000 | 0 / 0 (0 against 0) | 0 / 0 (0 against 0) | 7 / 8 (0.875) |
| 11 | div_num | 4 | 4 / 8 | 0.500 | 22 / 42 | 0.524 | 0 / 8 | 0.000 | 4 / 4 (1.000) | 22 / 22 (1.000) | 0 / 0 (0 against 0) |
| 11 | div_den | 5 | 5 / 5 | 1.000 | 26 / 26 | 1.000 | 0 / 0 | 0 against 0 | 5 / 5 (1.000) | 26 / 26 (1.000) | 0 / 0 (0 against 0) |
| 11 | div_general | 1872 | 3744 / 3744 | 1.000 | 64468 / 64468 | 1.000 | 3744 / 3744 | 1.000 | 3630 / 3744 (0.970) | 62515 / 64468 (0.970) | 3630 / 3744 (0.970) |

### L1-opt's tricks per job (spec §17.2), with the predicted figures and the share of L1's work removed

Each counter is summed over entry and the operations, beside the generator's prediction (spec §17.3 item 2; O-1 to O-5 must equal it exactly, by the gate; O-6 and O-7 are reported, not predicted). The share removed is the trick's skips over L1's count of the same work, entry and operations.

*Limitation:* L1-opt is timed with all its adopted tricks on together; the counters show each trick's effect on the work, but no trick's effect on time is measured alone (rule, Limitations: one row with every trick).

| job | trick | counters (predicted) | share of L1's work removed |
| --- | --- | --- | --- |
| 1 | O-1 | o1_skipped_add_den 585 (585), o1_skipped_mul_cross_ad 0 (0), o1_skipped_mul_cross_cb 0 (0), o1_skipped_div_num 0 (0), o1_skipped_div_den 0 (0) | lookups 0.006 |
| 1 | O-2 | o2_skipped0 0 (0), o2_skipped1 291 (291) | lookups 0.003 |
| 1 | O-3 | o3_gcd_skipped 391 (391), o3_gcd_work 2269 (2269) | gcds 0.009, gcd work 0.003 |
| 1 | O-4 | o4_div_skipped 29502 (29502), o4_mul_skipped 778 (778) | divisions 0.338, multiplications 0.017 |
| 1 | O-5 | o5_ops 0 (0), o5_add_skipped 0 (0), o5_gcd_skipped 0 (0), o5_gcd_work 0 (0), o5_div_skipped 0 (0) | gcds 0.000, gcd work 0.000, divisions 0.000, additions 0.000 |
| 1 | O-6 | o6_reads_avoided 24482 | - |
| 1 | O-7 | o7_growths_avoided 12, o7_bytes_avoided 262016 | - |
| 1 | all | | lookups 0.009, gcds 0.009, gcd work 0.003, divisions 0.338, multiplications 0.017, additions 0.000 |
| 2 | O-1 | o1_skipped_add_den 610 (610), o1_skipped_mul_cross_ad 0 (0), o1_skipped_mul_cross_cb 0 (0), o1_skipped_div_num 0 (0), o1_skipped_div_den 0 (0) | lookups 0.008 |
| 2 | O-2 | o2_skipped0 20 (20), o2_skipped1 347 (347) | lookups 0.005 |
| 2 | O-3 | o3_gcd_skipped 440 (440), o3_gcd_work 2568 (2568) | gcds 0.010, gcd work 0.005 |
| 2 | O-4 | o4_div_skipped 29682 (29682), o4_mul_skipped 873 (873) | divisions 0.341, multiplications 0.019 |
| 2 | O-5 | o5_ops 21 (21), o5_add_skipped 21 (21), o5_gcd_skipped 21 (21), o5_gcd_work 119 (119), o5_div_skipped 42 (42) | gcds 0.000, gcd work 0.000, divisions 0.000, additions 0.001 |
| 2 | O-6 | o6_reads_avoided 12361 | - |
| 2 | O-7 | o7_growths_avoided 12, o7_bytes_avoided 262016 | - |
| 2 | all | | lookups 0.013, gcds 0.010, gcd work 0.005, divisions 0.342, multiplications 0.019, additions 0.001 |
| 3 | O-1 | o1_skipped_add_den 0 (0), o1_skipped_mul_cross_ad 968 (968), o1_skipped_mul_cross_cb 846 (846), o1_skipped_div_num 918 (918), o1_skipped_div_den 938 (938) | lookups 0.061 |
| 3 | O-2 | o2_skipped0 0 (0), o2_skipped1 7180 (7180) | lookups 0.119 |
| 3 | O-3 | o3_gcd_skipped 9286 (9286), o3_gcd_work 145604 (145604) | gcds 0.243, gcd work 0.233 |
| 3 | O-4 | o4_div_skipped 59504 (59504), o4_mul_skipped 11078 (11078) | divisions 0.778, multiplications 0.337 |
| 3 | O-5 | o5_ops 0 (0), o5_add_skipped 0 (0), o5_gcd_skipped 0 (0), o5_gcd_work 0 (0), o5_div_skipped 0 (0) | gcds 0.000, gcd work 0.000, divisions 0.000, additions n/a (L1 has 0) |
| 3 | O-6 | o6_reads_avoided 16170 | - |
| 3 | O-7 | o7_growths_avoided 12, o7_bytes_avoided 262016 | - |
| 3 | all | | lookups 0.179, gcds 0.243, gcd work 0.233, divisions 0.778, multiplications 0.337, additions n/a |
| 4 | O-1 | o1_skipped_add_den 0 (0), o1_skipped_mul_cross_ad 118 (118), o1_skipped_mul_cross_cb 146 (146), o1_skipped_div_num 142 (142), o1_skipped_div_den 114 (114) | lookups 0.006 |
| 4 | O-2 | o2_skipped0 0 (0), o2_skipped1 541 (541) | lookups 0.006 |
| 4 | O-3 | o3_gcd_skipped 1520 (1520), o3_gcd_work 28167 (28167) | gcds 0.031, gcd work 0.029 |
| 4 | O-4 | o4_div_skipped 78662 (78662), o4_mul_skipped 1539 (1539) | divisions 0.805, multiplications 0.039 |
| 4 | O-5 | o5_ops 0 (0), o5_add_skipped 0 (0), o5_gcd_skipped 0 (0), o5_gcd_work 0 (0), o5_div_skipped 0 (0) | gcds 0.000, gcd work 0.000, divisions 0.000, additions n/a (L1 has 0) |
| 4 | O-6 | o6_reads_avoided 29710 | - |
| 4 | O-7 | o7_growths_avoided 12, o7_bytes_avoided 262016 | - |
| 4 | all | | lookups 0.011, gcds 0.031, gcd work 0.029, divisions 0.805, multiplications 0.039, additions n/a |
| 5 | O-1 | o1_skipped_add_den 5 (5), o1_skipped_mul_cross_ad 6 (6), o1_skipped_mul_cross_cb 2 (2), o1_skipped_div_num 0 (0), o1_skipped_div_den 4 (4) | lookups 0.000 |
| 5 | O-2 | o2_skipped0 0 (0), o2_skipped1 0 (0) | lookups 0.000 |
| 5 | O-3 | o3_gcd_skipped 0 (0), o3_gcd_work 0 (0) | gcds 0.000, gcd work 0.000 |
| 5 | O-4 | o4_div_skipped 51536 (51536), o4_mul_skipped 0 (0) | divisions 0.669, multiplications 0.000 |
| 5 | O-5 | o5_ops 0 (0), o5_add_skipped 0 (0), o5_gcd_skipped 0 (0), o5_gcd_work 0 (0), o5_div_skipped 0 (0) | gcds 0.000, gcd work 0.000, divisions 0.000, additions 0.000 |
| 5 | O-6 | o6_reads_avoided 40392 | - |
| 5 | O-7 | o7_growths_avoided 12, o7_bytes_avoided 262016 | - |
| 5 | all | | lookups 0.000, gcds 0.000, gcd work 0.000, divisions 0.669, multiplications 0.000, additions 0.000 |
| 6 | O-1 | o1_skipped_add_den 0 (0), o1_skipped_mul_cross_ad 2 (2), o1_skipped_mul_cross_cb 8 (8), o1_skipped_div_num 16 (16), o1_skipped_div_den 8 (8) | lookups 0.000 |
| 6 | O-2 | o2_skipped0 0 (0), o2_skipped1 0 (0) | lookups 0.000 |
| 6 | O-3 | o3_gcd_skipped 0 (0), o3_gcd_work 0 (0) | gcds 0.000, gcd work 0.000 |
| 6 | O-4 | o4_div_skipped 66246 (66246), o4_mul_skipped 0 (0) | divisions 0.663, multiplications 0.000 |
| 6 | O-5 | o5_ops 0 (0), o5_add_skipped 0 (0), o5_gcd_skipped 0 (0), o5_gcd_work 0 (0), o5_div_skipped 0 (0) | gcds 0.000, gcd work 0.000, divisions 0.000, additions n/a (L1 has 0) |
| 6 | O-6 | o6_reads_avoided 39395 | - |
| 6 | O-7 | o7_growths_avoided 12, o7_bytes_avoided 262016 | - |
| 6 | all | | lookups 0.000, gcds 0.000, gcd work 0.000, divisions 0.663, multiplications 0.000, additions n/a |
| 7 | O-1 | o1_skipped_add_den 4 (4), o1_skipped_mul_cross_ad 0 (0), o1_skipped_mul_cross_cb 0 (0), o1_skipped_div_num 0 (0), o1_skipped_div_den 0 (0) | lookups 0.000 |
| 7 | O-2 | o2_skipped0 0 (0), o2_skipped1 0 (0) | lookups 0.000 |
| 7 | O-3 | o3_gcd_skipped 0 (0), o3_gcd_work 0 (0) | gcds 0.000, gcd work 0.000 |
| 7 | O-4 | o4_div_skipped 35632 (35632), o4_mul_skipped 0 (0) | divisions 0.593, multiplications 0.000 |
| 7 | O-5 | o5_ops 0 (0), o5_add_skipped 0 (0), o5_gcd_skipped 0 (0), o5_gcd_work 0 (0), o5_div_skipped 0 (0) | gcds 0.000, gcd work 0.000, divisions 0.000, additions 0.000 |
| 7 | O-6 | o6_reads_avoided 39326 | - |
| 7 | O-7 | o7_growths_avoided 12, o7_bytes_avoided 262016 | - |
| 7 | all | | lookups 0.000, gcds 0.000, gcd work 0.000, divisions 0.593, multiplications 0.000, additions 0.000 |
| 8 | O-1 | o1_skipped_add_den 907 (907), o1_skipped_mul_cross_ad 0 (0), o1_skipped_mul_cross_cb 0 (0), o1_skipped_div_num 0 (0), o1_skipped_div_den 0 (0) | lookups 0.008 |
| 8 | O-2 | o2_skipped0 7 (7), o2_skipped1 460 (460) | lookups 0.004 |
| 8 | O-3 | o3_gcd_skipped 785 (785), o3_gcd_work 4583 (4583) | gcds 0.012, gcd work 0.005 |
| 8 | O-4 | o4_div_skipped 40920 (40920), o4_mul_skipped 1554 (1554) | divisions 0.322, multiplications 0.021 |
| 8 | O-5 | o5_ops 8 (8), o5_add_skipped 8 (8), o5_gcd_skipped 8 (8), o5_gcd_work 53 (53), o5_div_skipped 16 (16) | gcds 0.000, gcd work 0.000, divisions 0.000, additions 0.000 |
| 8 | O-6 | o6_reads_avoided 19494 | - |
| 8 | O-7 | o7_growths_avoided 13, o7_bytes_avoided 524160 | - |
| 8 | all | | lookups 0.012, gcds 0.012, gcd work 0.006, divisions 0.322, multiplications 0.021, additions 0.000 |
| 9 | O-1 | o1_skipped_add_den 7 (7), o1_skipped_mul_cross_ad 0 (0), o1_skipped_mul_cross_cb 0 (0), o1_skipped_div_num 0 (0), o1_skipped_div_den 0 (0) | lookups 0.000 |
| 9 | O-2 | o2_skipped0 37 (37), o2_skipped1 331 (331) | lookups 0.003 |
| 9 | O-3 | o3_gcd_skipped 131 (131), o3_gcd_work 2138 (2138) | gcds 0.002, gcd work 0.002 |
| 9 | O-4 | o4_div_skipped 53674 (53674), o4_mul_skipped 1242 (1242) | divisions 0.368, multiplications 0.013 |
| 9 | O-5 | o5_ops 0 (0), o5_add_skipped 0 (0), o5_gcd_skipped 0 (0), o5_gcd_work 0 (0), o5_div_skipped 0 (0) | gcds 0.000, gcd work 0.000, divisions 0.000, additions 0.000 |
| 9 | O-6 | o6_reads_avoided 18783 | - |
| 9 | O-7 | o7_growths_avoided 13, o7_bytes_avoided 524160 | - |
| 9 | all | | lookups 0.003, gcds 0.002, gcd work 0.002, divisions 0.368, multiplications 0.013, additions 0.000 |
| 10 | O-1 | o1_skipped_add_den 0 (0), o1_skipped_mul_cross_ad 16 (16), o1_skipped_mul_cross_cb 14 (14), o1_skipped_div_num 12 (12), o1_skipped_div_den 26 (26) | lookups 0.001 |
| 10 | O-2 | o2_skipped0 0 (0), o2_skipped1 502 (502) | lookups 0.010 |
| 10 | O-3 | o3_gcd_skipped 482 (482), o3_gcd_work 6656 (6656) | gcds 0.019, gcd work 0.022 |
| 10 | O-4 | o4_div_skipped 40416 (40416), o4_mul_skipped 483 (483) | divisions 0.781, multiplications 0.030 |
| 10 | O-5 | o5_ops 0 (0), o5_add_skipped 0 (0), o5_gcd_skipped 0 (0), o5_gcd_work 0 (0), o5_div_skipped 0 (0) | gcds 0.000, gcd work 0.000, divisions 0.000, additions n/a (L1 has 0) |
| 10 | O-6 | o6_reads_avoided 7923 | - |
| 10 | O-7 | o7_growths_avoided 12, o7_bytes_avoided 262016 | - |
| 10 | all | | lookups 0.011, gcds 0.019, gcd work 0.022, divisions 0.781, multiplications 0.030, additions n/a |
| 11 | O-1 | o1_skipped_add_den 9 (9), o1_skipped_mul_cross_ad 14 (14), o1_skipped_mul_cross_cb 2 (2), o1_skipped_div_num 6 (6), o1_skipped_div_den 10 (10) | lookups 0.001 |
| 11 | O-2 | o2_skipped0 1 (1), o2_skipped1 511 (511) | lookups 0.010 |
| 11 | O-3 | o3_gcd_skipped 330 (330), o3_gcd_work 5379 (5379) | gcds 0.014, gcd work 0.018 |
| 11 | O-4 | o4_div_skipped 33660 (33660), o4_mul_skipped 576 (576) | divisions 0.786, multiplications 0.029 |
| 11 | O-5 | o5_ops 1 (1), o5_add_skipped 1 (1), o5_gcd_skipped 1 (1), o5_gcd_work 3 (3), o5_div_skipped 2 (2) | gcds 0.000, gcd work 0.000, divisions 0.000, additions 0.000 |
| 11 | O-6 | o6_reads_avoided 8524 | - |
| 11 | O-7 | o7_growths_avoided 12, o7_bytes_avoided 262016 | - |
| 11 | all | | lookups 0.011, gcds 0.014, gcd work 0.018, divisions 0.786, multiplications 0.029, additions 0.000 |

### Entry against operations; store's share (rule item 12)

Per row, the share of the median total time spent in entry. For L1 and L1-opt, the share of gcd work and of lookups counted in `store` (entry and operations). `store` does no arithmetic (spec §6: it only interns the reduced numerator and denominator), so its share of gcd work is 0 by construction: every gcd, entry's reduction included, is counted in the core. Likewise every lookup is made by `store`'s intern, so its share of lookups is 1 by construction. Both are computed anyway, so a departure would show.

- job 1: entry share L1 0.259; L1-opt 0.261; L1-Q32 0.259; L1-opt-Q32 0.263; ctl 0.239; fmpq 0.233; mpq 0.156; fz 0.052; L1 store share of gcd work 0 (by construction), of lookups 1.000; L1-opt store share of gcd work 0 (by construction), of lookups 1.000
- job 2: entry share L1 0.200; L1-opt 0.206; L1-Q32 0.205; L1-opt-Q32 0.204; ctl 0.220; fmpq 0.214; mpq 0.150; fz 0.051; L1 store share of gcd work 0 (by construction), of lookups 1.000; L1-opt store share of gcd work 0 (by construction), of lookups 1.000
- job 3: entry share L1 0.250; L1-opt 0.246; L1-Q32 0.251; L1-opt-Q32 0.251; ctl 0.184; fmpq 0.168; mpq 0.134; L1 store share of gcd work 0 (by construction), of lookups 1.000; L1-opt store share of gcd work 0 (by construction), of lookups 1.000
- job 4: entry share L1 0.166; L1-opt 0.151; L1-Q32 0.169; L1-opt-Q32 0.152; ctl 0.159; fmpq 0.155; mpq 0.126; L1 store share of gcd work 0 (by construction), of lookups 1.000; L1-opt store share of gcd work 0 (by construction), of lookups 1.000
- job 5: entry share L1 0.171; L1-opt 0.151; ctl 0.173; fmpq 0.162; mpq 0.142; L1 store share of gcd work 0 (by construction), of lookups 1.000; L1-opt store share of gcd work 0 (by construction), of lookups 1.000
- job 6: entry share L1 0.165; L1-opt 0.144; ctl 0.165; fmpq 0.160; mpq 0.133; L1 store share of gcd work 0 (by construction), of lookups 1.000; L1-opt store share of gcd work 0 (by construction), of lookups 1.000
- job 7: entry share L1 0.200; L1-opt 0.179; ctl 0.182; fmpq 0.174; mpq 0.164; L1 store share of gcd work 0 (by construction), of lookups 1.000; L1-opt store share of gcd work 0 (by construction), of lookups 1.000
- job 8: entry share L1 0.140; L1-opt 0.142; L1-Q32 0.144; L1-opt-Q32 0.141; ctl 0.150; fmpq 0.148; mpq 0.110; fz 0.036; L1 store share of gcd work 0 (by construction), of lookups 1.000; L1-opt store share of gcd work 0 (by construction), of lookups 1.000
- job 9: entry share L1 0.124; L1-opt 0.118; L1-Q32 0.125; L1-opt-Q32 0.119; ctl 0.117; fmpq 0.109; mpq 0.083; L1 store share of gcd work 0 (by construction), of lookups 1.000; L1-opt store share of gcd work 0 (by construction), of lookups 1.000
- job 10: entry share L1 0.238; L1-opt 0.224; L1-Q32 0.240; L1-opt-Q32 0.223; ctl 0.274; fmpq 0.260; mpq 0.253; L1 store share of gcd work 0 (by construction), of lookups 1.000; L1-opt store share of gcd work 0 (by construction), of lookups 1.000
- job 11: entry share L1 0.236; L1-opt 0.225; L1-Q32 0.242; L1-opt-Q32 0.224; ctl 0.269; fmpq 0.251; mpq 0.258; L1 store share of gcd work 0 (by construction), of lookups 1.000; L1-opt store share of gcd work 0 (by construction), of lookups 1.000

### Counts per operation (rule item 12)

Totals per phase (entry, then each operation), core and store together; every counter per phase, core and store is in `counters.csv`.

| job | row | phase | n | gcd | gcd_work | mul | add | div_exact | lookup_search | lookup_probe | intern_found | intern_appended | idx_cmp | cases |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | L1 | entry | 10000 | 10000 | 223172 | 0 | 0 | 20000 | 36387 | 47299 | 10009 | 9991 | 0 | - |
| 1 | L1 | add | 9945 | 17154 | 263395 | 22593 | 9945 | 33664 | 36275 | 47235 | 10008 | 9882 | 39780 | den 2414, general 7531 |
| 1 | L1 | sub | 10055 | 17212 | 265441 | 22533 | 10055 | 33716 | 20110 | 28373 | 10117 | 9993 | 40220 | den 2544, general 7511 |
| 1 | L1-opt | entry | 10000 | 10000 | 223172 | 0 | 0 | 12014 | 36287 | 47199 | 9909 | 9991 | 0 | - |
| 1 | L1-opt | add | 9945 | 16966 | 262335 | 22217 | 9945 | 22784 | 35902 | 46862 | 9635 | 9882 | 39780 | den 2414, general 7531 |
| 1 | L1-opt | sub | 10055 | 17009 | 264232 | 22131 | 10055 | 23080 | 19707 | 27970 | 9714 | 9993 | 40220 | den 2544, general 7511 |
| 1 | ctl | entry | 10000 | 10000 | 223172 | 0 | 0 | 20000 | 0 | 0 | 0 | 0 | 0 | - |
| 1 | ctl | add | 9945 | 17154 | 263395 | 22593 | 9945 | 33664 | 0 | 0 | 0 | 0 | 0 | - |
| 1 | ctl | sub | 10055 | 17212 | 265441 | 22533 | 10055 | 33716 | 0 | 0 | 0 | 0 | 0 | - |
| 2 | L1 | entry | 10000 | 10000 | 156708 | 0 | 0 | 20000 | 20253 | 23524 | 19791 | 209 | 0 | - |
| 2 | L1 | add | 10038 | 17111 | 199377 | 22374 | 10038 | 33452 | 31600 | 37439 | 12424 | 7652 | 40152 | den 2580, general 7458 |
| 2 | L1 | sub | 9962 | 17114 | 198583 | 22563 | 9962 | 33490 | 24534 | 29672 | 12382 | 7542 | 39848 | den 2441, general 7521 |
| 2 | L1-opt | entry | 10000 | 10000 | 156708 | 0 | 0 | 12060 | 20147 | 23418 | 19685 | 209 | 0 | - |
| 2 | L1-opt | add | 10038 | 16881 | 198014 | 21939 | 10028 | 22572 | 31147 | 36986 | 11971 | 7652 | 40152 | den 2580, general 7458 |
| 2 | L1-opt | sub | 9962 | 16883 | 197259 | 22125 | 9951 | 22586 | 24116 | 29254 | 11964 | 7542 | 39848 | den 2441, general 7521 |
| 2 | ctl | entry | 10000 | 10000 | 156708 | 0 | 0 | 20000 | 0 | 0 | 0 | 0 | 0 | - |
| 2 | ctl | add | 10038 | 17111 | 199377 | 22374 | 10038 | 33452 | 0 | 0 | 0 | 0 | 0 | - |
| 2 | ctl | sub | 9962 | 17114 | 198583 | 22563 | 9962 | 33490 | 0 | 0 | 0 | 0 | 0 | - |
| 3 | L1 | entry | 10000 | 10000 | 133972 | 0 | 0 | 20000 | 20000 | 33180 | 19992 | 8 | 0 | - |
| 3 | L1 | mul | 9902 | 13973 | 242179 | 16238 | 0 | 27946 | 19975 | 21416 | 19649 | 155 | 39608 | both 182, den 1075, num 858, cross_ad 837, cross_cb 764, general 6186 |
| 3 | L1 | div | 10098 | 14282 | 249094 | 16634 | 0 | 28564 | 20535 | 22198 | 20009 | 187 | 40392 | both 195, cross_ad 1088, cross_cb 881, num 784, den 802, general 6348 |
| 3 | L1-opt | entry | 10000 | 10000 | 133972 | 0 | 0 | 0 | 15465 | 28645 | 15457 | 8 | 0 | - |
| 3 | L1-opt | mul | 9902 | 9407 | 170640 | 10726 | 0 | 8524 | 16829 | 18258 | 16503 | 155 | 39608 | both 182, den 1075, num 858, cross_ad 837, cross_cb 764, general 6186 |
| 3 | L1-opt | div | 10098 | 9562 | 175029 | 11068 | 0 | 8482 | 17366 | 19020 | 16840 | 187 | 40392 | both 195, cross_ad 1088, cross_cb 881, num 784, den 802, general 6348 |
| 3 | ctl | entry | 10000 | 10000 | 133972 | 0 | 0 | 20000 | 0 | 0 | 0 | 0 | 0 | - |
| 3 | ctl | mul | 9902 | 17642 | 285189 | 19804 | 0 | 35284 | 0 | 0 | 0 | 0 | 0 | - |
| 3 | ctl | div | 10098 | 19118 | 313752 | 18040 | 0 | 38236 | 0 | 0 | 0 | 0 | 0 | - |
| 4 | L1 | entry | 10000 | 10000 | 148139 | 0 | 0 | 20000 | 20059 | 31525 | 19936 | 64 | 0 | - |
| 4 | L1 | mul | 10002 | 19400 | 404638 | 19628 | 0 | 38800 | 40999 | 53808 | 9321 | 10683 | 40008 | both 2, den 105, num 102, cross_ad 85, cross_cb 101, general 9607 |
| 4 | L1 | div | 9998 | 19438 | 404026 | 19614 | 0 | 38876 | 31714 | 43147 | 9397 | 10599 | 39992 | both 3, cross_ad 75, cross_cb 107, num 102, den 86, general 9625 |
| 4 | L1-opt | entry | 10000 | 10000 | 148139 | 0 | 0 | 0 | 19551 | 30505 | 19428 | 64 | 0 | - |
| 4 | L1-opt | mul | 10002 | 18637 | 390309 | 18853 | 0 | 9514 | 40719 | 53526 | 9041 | 10683 | 40008 | both 2, den 105, num 102, cross_ad 85, cross_cb 101, general 9607 |
| 4 | L1-opt | div | 9998 | 18681 | 390188 | 18850 | 0 | 9500 | 31441 | 42874 | 9124 | 10599 | 39992 | both 3, cross_ad 75, cross_cb 107, num 102, den 86, general 9625 |
| 4 | ctl | entry | 10000 | 10000 | 148139 | 0 | 0 | 20000 | 0 | 0 | 0 | 0 | 0 | - |
| 4 | ctl | mul | 10002 | 19794 | 410352 | 20004 | 0 | 39588 | 0 | 0 | 0 | 0 | 0 | - |
| 4 | ctl | div | 9998 | 19907 | 411083 | 19818 | 0 | 39814 | 0 | 0 | 0 | 0 | 0 | - |
| 5 | L1 | entry | 10000 | 10000 | 198257 | 0 | 0 | 20000 | 21023 | 31048 | 19000 | 1000 | 0 | - |
| 5 | L1 | add | 5062 | 7201 | 241057 | 15177 | 5062 | 8584 | 11149 | 18836 | 37 | 10087 | 20248 | cross 5, den 2, num 3, general 5052 |
| 5 | L1 | sub | 4932 | 7063 | 235182 | 14783 | 4932 | 8536 | 9864 | 17367 | 32 | 9832 | 19728 | cross 1, den 4, num 1, general 4926 |
| 5 | L1 | mul | 5103 | 10188 | 285783 | 10190 | 0 | 20376 | 47072 | 61328 | 47 | 10159 | 20412 | den 4, num 1, cross_ad 4, cross_cb 4, general 5090 |
| 5 | L1 | div | 4903 | 9796 | 277081 | 9802 | 0 | 19592 | 36433 | 48411 | 35 | 9771 | 19612 | cross_ad 2, cross_cb 2, den 2, general 4897 |
| 5 | L1-opt | entry | 10000 | 10000 | 198257 | 0 | 0 | 0 | 21023 | 31048 | 19000 | 1000 | 0 | - |
| 5 | L1-opt | add | 5062 | 7201 | 241057 | 15177 | 5062 | 4962 | 11147 | 18834 | 35 | 10087 | 20248 | cross 5, den 2, num 3, general 5052 |
| 5 | L1-opt | sub | 4932 | 7063 | 235182 | 14783 | 4932 | 4990 | 9861 | 17364 | 29 | 9832 | 19728 | cross 1, den 4, num 1, general 4926 |
| 5 | L1-opt | mul | 5103 | 10188 | 285783 | 10190 | 0 | 7776 | 47064 | 61320 | 39 | 10159 | 20412 | den 4, num 1, cross_ad 4, cross_cb 4, general 5090 |
| 5 | L1-opt | div | 4903 | 9796 | 277081 | 9802 | 0 | 7824 | 36429 | 48407 | 31 | 9771 | 19612 | cross_ad 2, cross_cb 2, den 2, general 4897 |
| 5 | ctl | entry | 10000 | 10000 | 198257 | 0 | 0 | 20000 | 0 | 0 | 0 | 0 | 0 | - |
| 5 | ctl | add | 5062 | 7206 | 241194 | 15180 | 5062 | 8580 | 0 | 0 | 0 | 0 | 0 | - |
| 5 | ctl | sub | 4932 | 7064 | 235222 | 14784 | 4932 | 8536 | 0 | 0 | 0 | 0 | 0 | - |
| 5 | ctl | mul | 5103 | 10198 | 285979 | 10206 | 0 | 20396 | 0 | 0 | 0 | 0 | 0 | - |
| 5 | ctl | div | 4903 | 9804 | 277241 | 9802 | 0 | 19608 | 0 | 0 | 0 | 0 | 0 | - |
| 6 | L1 | entry | 10000 | 10000 | 198432 | 0 | 0 | 20000 | 21023 | 30573 | 19000 | 1000 | 0 | - |
| 6 | L1 | mul | 9987 | 19950 | 557438 | 19958 | 0 | 39900 | 42505 | 61255 | 99 | 19875 | 39948 | den 3, num 5, cross_ad 3, cross_cb 5, general 9971 |
| 6 | L1 | div | 10013 | 19993 | 556277 | 19992 | 0 | 39986 | 62013 | 83756 | 103 | 19923 | 40052 | cross_ad 6, cross_cb 2, num 8, den 9, general 9988 |
| 6 | L1-opt | entry | 10000 | 10000 | 198432 | 0 | 0 | 0 | 21023 | 30573 | 19000 | 1000 | 0 | - |
| 6 | L1-opt | mul | 9987 | 19950 | 557438 | 19958 | 0 | 16886 | 42495 | 61245 | 89 | 19875 | 39948 | den 3, num 5, cross_ad 3, cross_cb 5, general 9971 |
| 6 | L1-opt | div | 10013 | 19993 | 556277 | 19992 | 0 | 16754 | 61989 | 83731 | 79 | 19923 | 40052 | cross_ad 6, cross_cb 2, num 8, den 9, general 9988 |
| 6 | ctl | entry | 10000 | 10000 | 198432 | 0 | 0 | 20000 | 0 | 0 | 0 | 0 | 0 | - |
| 6 | ctl | mul | 9987 | 19968 | 557793 | 19974 | 0 | 39936 | 0 | 0 | 0 | 0 | 0 | - |
| 6 | ctl | div | 10013 | 20017 | 556751 | 20008 | 0 | 40034 | 0 | 0 | 0 | 0 | 0 | - |
| 7 | L1 | entry | 10000 | 10000 | 198123 | 0 | 0 | 20000 | 36387 | 48350 | 9071 | 10929 | 0 | - |
| 7 | L1 | add | 20000 | 30025 | 1026524 | 59985 | 20000 | 40110 | 89154 | 127099 | 134 | 39866 | 80000 | den 5, general 19995 |
| 7 | L1-opt | entry | 10000 | 10000 | 198123 | 0 | 0 | 0 | 36387 | 48350 | 9071 | 10929 | 0 | - |
| 7 | L1-opt | add | 20000 | 30025 | 1026524 | 59985 | 20000 | 24478 | 89150 | 127095 | 130 | 39866 | 80000 | den 5, general 19995 |
| 7 | ctl | entry | 10000 | 10000 | 198123 | 0 | 0 | 20000 | 0 | 0 | 0 | 0 | 0 | - |
| 7 | ctl | add | 20000 | 30025 | 1026524 | 59985 | 20000 | 40110 | 0 | 0 | 0 | 0 | 0 | - |
| 8 | L1 | entry | 10000 | 10000 | 158823 | 0 | 0 | 20000 | 20253 | 23396 | 19792 | 208 | 0 | - |
| 8 | L1 | add | 16084 | 27661 | 340565 | 36834 | 16084 | 53920 | 56237 | 67064 | 18275 | 13893 | 64336 | den 3806, general 12278 |
| 8 | L1 | sub | 15916 | 27242 | 336267 | 36129 | 15916 | 53050 | 40282 | 49399 | 18052 | 13780 | 63664 | den 3873, general 12043 |
| 8 | L1-opt | entry | 10000 | 10000 | 158823 | 0 | 0 | 12248 | 20112 | 23255 | 19651 | 208 | 0 | - |
| 8 | L1-opt | add | 16084 | 27255 | 338171 | 36040 | 16081 | 37214 | 55627 | 66454 | 17665 | 13893 | 64336 | den 3806, general 12278 |
| 8 | L1-opt | sub | 15916 | 26855 | 334025 | 35369 | 15911 | 36572 | 39659 | 48776 | 17429 | 13780 | 63664 | den 3873, general 12043 |
| 8 | ctl | entry | 10000 | 10000 | 158823 | 0 | 0 | 20000 | 0 | 0 | 0 | 0 | 0 | - |
| 8 | ctl | add | 16084 | 27661 | 340565 | 36834 | 16084 | 53920 | 0 | 0 | 0 | 0 | 0 | - |
| 8 | ctl | sub | 15916 | 27242 | 336267 | 36129 | 15916 | 53050 | 0 | 0 | 0 | 0 | 0 | - |
| 9 | L1 | entry | 10000 | 10000 | 102189 | 0 | 0 | 20000 | 24097 | 26612 | 16732 | 3268 | 0 | - |
| 9 | L1 | add | 15965 | 31670 | 553495 | 47630 | 15965 | 63000 | 31930 | 40034 | 19405 | 12525 | 63860 | cross 1, den 88, num 1, general 15875 |
| 9 | L1 | sub | 16035 | 31759 | 553215 | 47899 | 16035 | 63048 | 60745 | 72415 | 19484 | 12586 | 64140 | cross 2, den 68, num 2, general 15963 |
| 9 | L1-opt | entry | 10000 | 10000 | 102189 | 0 | 0 | 0 | 23732 | 26237 | 16367 | 3268 | 0 | - |
| 9 | L1-opt | add | 15965 | 31613 | 552557 | 47032 | 15965 | 46104 | 31923 | 40027 | 19398 | 12525 | 63860 | cross 1, den 88, num 1, general 15875 |
| 9 | L1-opt | sub | 16035 | 31685 | 552015 | 47255 | 16035 | 46270 | 60742 | 72412 | 19481 | 12586 | 64140 | cross 2, den 68, num 2, general 15963 |
| 9 | ctl | entry | 10000 | 10000 | 102189 | 0 | 0 | 20000 | 0 | 0 | 0 | 0 | 0 | - |
| 9 | ctl | add | 15965 | 31671 | 553507 | 47631 | 15965 | 63000 | 0 | 0 | 0 | 0 | 0 | - |
| 9 | ctl | sub | 16035 | 31761 | 553228 | 47901 | 16035 | 63040 | 0 | 0 | 0 | 0 | 0 | - |
| 10 | L1 | entry | 10000 | 10000 | 56732 | 0 | 0 | 20000 | 20059 | 20072 | 19937 | 63 | 0 | - |
| 10 | L1 | mul | 4018 | 7976 | 122178 | 7996 | 0 | 15952 | 16680 | 21880 | 2575 | 5461 | 16072 | den 11, num 9, cross_ad 10, cross_cb 10, general 3978 |
| 10 | L1 | div | 3982 | 7892 | 121509 | 7916 | 0 | 15784 | 15648 | 20459 | 2543 | 5421 | 15928 | cross_ad 17, cross_cb 7, num 10, den 14, general 3934 |
| 10 | L1-opt | entry | 10000 | 10000 | 56732 | 0 | 0 | 0 | 19568 | 19581 | 19446 | 63 | 0 | - |
| 10 | L1-opt | mul | 4018 | 7743 | 119002 | 7762 | 0 | 5694 | 16645 | 21845 | 2540 | 5461 | 16072 | den 11, num 9, cross_ad 10, cross_cb 10, general 3978 |
| 10 | L1-opt | div | 3982 | 7643 | 118029 | 7667 | 0 | 5626 | 15604 | 20415 | 2499 | 5421 | 15928 | cross_ad 17, cross_cb 7, num 10, den 14, general 3934 |
| 10 | ctl | entry | 10000 | 10000 | 56732 | 0 | 0 | 20000 | 0 | 0 | 0 | 0 | 0 | - |
| 10 | ctl | mul | 4018 | 8014 | 122398 | 8036 | 0 | 16028 | 0 | 0 | 0 | 0 | 0 | - |
| 10 | ctl | div | 3982 | 7950 | 121839 | 7936 | 0 | 15900 | 0 | 0 | 0 | 0 | 0 | - |
| 11 | L1 | entry | 10000 | 10000 | 56712 | 0 | 0 | 20000 | 20059 | 20073 | 19937 | 63 | 0 | - |
| 11 | L1 | add | 2032 | 2918 | 54084 | 6079 | 2032 | 3596 | 4577 | 6844 | 767 | 3297 | 8128 | cross 8, den 4, num 5, general 2015 |
| 11 | L1 | sub | 1968 | 2802 | 52305 | 5878 | 1968 | 3382 | 10211 | 12985 | 678 | 3258 | 7872 | cross 4, den 7, num 5, general 1952 |
| 11 | L1 | mul | 2111 | 4172 | 72292 | 4186 | 0 | 8344 | 13762 | 17166 | 1027 | 3195 | 8444 | both 2, den 8, num 7, cross_ad 11, cross_cb 5, general 2078 |
| 11 | L1 | div | 1889 | 3753 | 64516 | 3760 | 0 | 7506 | 3778 | 5719 | 909 | 2869 | 7556 | cross_ad 4, cross_cb 4, num 4, den 5, general 1872 |
| 11 | L1-opt | entry | 10000 | 10000 | 56712 | 0 | 0 | 0 | 19560 | 19574 | 19438 | 63 | 0 | - |
| 11 | L1-opt | add | 2032 | 2852 | 53068 | 5880 | 2032 | 2142 | 4572 | 6839 | 762 | 3297 | 8128 | cross 8, den 4, num 5, general 2015 |
| 11 | L1-opt | sub | 1968 | 2755 | 51554 | 5720 | 1967 | 2020 | 10205 | 12979 | 672 | 3258 | 7872 | cross 4, den 7, num 5, general 1952 |
| 11 | L1-opt | mul | 2111 | 4068 | 70630 | 4082 | 0 | 2692 | 13737 | 17141 | 1002 | 3195 | 8444 | both 2, den 8, num 7, cross_ad 11, cross_cb 5, general 2078 |
| 11 | L1-opt | div | 1889 | 3639 | 62563 | 3645 | 0 | 2312 | 3760 | 5701 | 891 | 2869 | 7556 | cross_ad 4, cross_cb 4, num 4, den 5, general 1872 |
| 11 | ctl | entry | 10000 | 10000 | 56712 | 0 | 0 | 20000 | 0 | 0 | 0 | 0 | 0 | - |
| 11 | ctl | add | 2032 | 2926 | 54141 | 6084 | 2032 | 3584 | 0 | 0 | 0 | 0 | 0 | - |
| 11 | ctl | sub | 1968 | 2806 | 52334 | 5883 | 1968 | 3366 | 0 | 0 | 0 | 0 | 0 | - |
| 11 | ctl | mul | 2111 | 4206 | 72466 | 4222 | 0 | 8412 | 0 | 0 | 0 | 0 | 0 | - |
| 11 | ctl | div | 1889 | 3773 | 64632 | 3768 | 0 | 7546 | 0 | 0 | 0 | 0 | 0 | - |

### Chosen forms (fmpq, mpq)

- 1:fmpq: form A (median A 1.702 ms, B 1.709 ms)
- 1:mpq: form B (median A 4.223 ms, B 3.898 ms)
- 2:fmpq: form A (median A 1.462 ms, B 1.489 ms)
- 2:mpq: form B (median A 4.385 ms, B 4.161 ms)
- 3:fmpq: form A (median A 1.529 ms, B 1.568 ms)
- 3:mpq: form B (median A 4.546 ms, B 4.149 ms)
- 4:fmpq: form A (median A 2.249 ms, B 2.250 ms)
- 4:mpq: form B (median A 4.813 ms, B 4.531 ms)
- 5:fmpq: form B (median A 2.722 ms, B 2.704 ms)
- 5:mpq: form B (median A 5.255 ms, B 4.756 ms)
- 6:fmpq: form B (median A 2.779 ms, B 2.773 ms)
- 6:mpq: form B (median A 5.305 ms, B 5.135 ms)
- 7:fmpq: form B (median A 2.539 ms, B 2.521 ms)
- 7:mpq: form B (median A 4.272 ms, B 4.022 ms)
- 8:fmpq: form B (median A 2.255 ms, B 2.178 ms)
- 8:mpq: form B (median A 6.105 ms, B 5.552 ms)
- 9:fmpq: form B (median A 2.759 ms, B 2.724 ms)
- 9:mpq: form B (median A 6.490 ms, B 6.061 ms)
- 10:fmpq: form B (median A 0.773 ms, B 0.766 ms)
- 10:mpq: form B (median A 1.898 ms, B 1.685 ms)
- 11:fmpq: form B (median A 0.804 ms, B 0.797 ms)
- 11:mpq: form B (median A 1.882 ms, B 1.685 ms)

## Limitations (rule, Limitations)

- **Keeping every result** (spec §10 item 3): every row stores every result as a new element; on the chain jobs (8-11) this is not the libraries' normal use, so it is noted beside every chain verdict against fmpq, mpq and the best library.
- **One row with every trick:** L1-opt is timed with all its adopted tricks on together; the counters show each trick's effect on the work, but no trick's effect on time is measured alone (rule, Limitations: one row with every trick).
- **Designed after the Q64 results:** The Q32 rows were designed after the Q64 results of commit b3b88b6 had been seen (rule, revision of Oct 29, 2026: an exception approved by the author to "only while nothing has been measured"). The official Q64 verdicts, against ctl and the libraries and of L1-opt against L1, stay those of b3b88b6 ([archived results](../results-q64-b3b88b6/results.md)); the Q64 rows re-run here are used only for the paired Q32-against-Q64 comparison and for the band h of the same run (L1 against L1-null, which must come from the same rounds as the verdicts it bands), and every Q64 verdict that differs between the two runs is listed below, without choosing between them.

