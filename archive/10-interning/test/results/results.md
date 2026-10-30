# Design 3, layer 1: results

Run started 2026-10-29T08:18:29-05:00.

## The measurement rule (research/rule.md, verbatim, written before the first trial)

sha256 `eb631a463d99f14dd278c08dc95a9075099ed8f012b906c9da7e195b5ccbd26d`

<!-- rule begins -->
# Design 3, layer 1: pre-registered measurement rule

Oct 29, 2026 · **Fixed.** Every threshold, count and limit below is **approved by the author, Oct 29, 2026**, and so is every workload parameter, including the 60-minute hard stop, the 45-minute pilot limit and the 2.0 severe-memory-loss flag. The rule is fixed before any measurement: the harness writes it verbatim into its results file before the first trial, and refuses a measured run unless this file holds that text (as 08 harness R-n, by sha256). A change after that is a revision, dated and explained at the top, and allowed only while nothing has been measured.

*Revised Oct 29, 2026, after the first review and at the author's direction:* a second Design 3 row, **L1-opt** (spec §17), with its own exact checks; gcd and multiplication ratios per case, and an exact check that case `den` and the general case do the control's work (item 6); library holder arrays charged for their stored elements only (item 6); the hard stop set to **60 minutes**, and the pilot's projection limit to 45 (Run plan); the severe-memory-loss threshold justified (item 8); keeping every result reported as a limitation on chain jobs (Limitations).

*Revised Oct 29, 2026, after the independent review, at the author's direction; procedure only, no threshold, count, measure or report changes. Nothing has been measured: only the pilot has run, and its results are discarded (Run plan step 3).* **Idle machine.** Before any timed run (the pilot, and each job's calibration and rounds in step 6), the harness checks that the machine is idle: the 1-minute load average at most **0.5**, and no other process using more than **5%** of one core over a 1-second sample. If it is not, the harness re-checks every 15 seconds, logging each check, and after **10 minutes** refuses to time and stops the run, reported to the author. The wait counts on the run's own clock. This is the check added to 08 at the same time; it makes item 4's "nothing else runs during a timed invocation" checked rather than assumed.

It states how layer 1 of [spec v0.1](../v0.1.md), as the baseline L1 and as the optimized variant L1-opt, is measured against the control of 08, FLINT `fmpq`, GMP `mpq_t` and a FLINT shared-denominator ceiling, and how the results are reported: **per measure only, with no overall winner** (item 11).

**This comparison does not end the debate.** L1 is a faithful baseline of the paper's combined structure, with every later layer and every optimization left out on purpose (spec §11); L1-opt adds only the production tricks of spec §17, within the author's exclusions, so it still has no later layer. At 64 bits a gcd is cheap (07 M3 found the bookkeeping of sharing outweighing it there), so a gcd removed here is worth less than it will be at larger sizes; the gcd-work and multiplication counts are reported for that reason.

## Measurement rule

1. **Rows.** Each job runs these rows:
   - **L1**: layer 1, the baseline (spec §§4–15);
   - **L1-opt**: layer 1 with the tricks of spec §17 that the author adopts, every one on; a separate binary, run on the same workloads;
   - **L1-null**: a second copy of L1, the same binary run as a separate row (the null control);
   - **ctl**: the 08 control, plain reduced rationals (spec §9);
   - **fmpq**: FLINT rationals, and **mpq**: GMP rationals, each in its best normal form (spec §10; the form is calibrated by item 4);
   - **fz**: the FLINT shared-denominator ceiling, only on the jobs marked "fz" in the run plan, which are fully shared: every value is entered over one denominator and every operation is an addition or subtraction, so every result stays over it (as M3's fz).

2. **Builds.** L1, L1-opt, ctl and the driver are built by one compiler with one set of flags (`gcc -std=c11 -O2`), at width 64, with the 08 integer module and the 08 control unchanged (their sha256 recorded, as 08 harness R-o). FLINT and GMP are the system libraries as installed; their versions and `flint_get_num_threads()` (which MUST be 1) are written into every output line and the results file. The timing build carries no counters, audits or allocation hooks. The **counting build** gives the counts and memory reports of L1, L1-opt and ctl, from one complete run per row and job; in it the driver reads every row's counters after every call, untimed, so that each operation's counts can be attributed to its predicted case (spec §14.1 item 3). The **memory run** (item 6) gives every row's heap figures. Counts and memory figures are deterministic: on jobs 3, 5 and 10 every counting run and every memory run is repeated, and the second MUST give identical figures, or it is a bug (item 5). Every counting or memory run has a limit of **10 minutes**; one that does not finish gives n/a, with the reason. The test build gives the audits. L1-null shares L1's gate, counting and memory runs, being the same binary.

3. **Gates before the measured run.** In the test build, L1, L1-opt and ctl MUST pass their unit tests (spec §14.3 item 1 and §17.3; 08's for ctl) and, on every job, the gate of spec §14.3 item 2. L1's and L1-opt's gate runs MUST also pass the two exact checks of spec §14.1 items 1 and 2: W against the reference, after entry and at the end, and the count of every case against the generator's prediction. L1-opt's MUST also pass the checks of spec §17.3 items 1–3: its stored values and whole-number list equal to L1's, every predicted trick counter (O-1 skipped lookups per case, and O-2 to O-5 if adopted) equal to the prediction, and its lookups and arithmetic counts equal to L1's less its counted skips. The counting runs MUST pass the check of spec §14.1 item 3: on cases `den` and `general`, L1's arithmetic equal to ctl's. The library rows have no gate, but every one of their runs is checked (item 5). A row that fails a gate is fixed and the gate re-run; no measured run starts until every gate passes. Gates are untimed and MAY run two at a time, one per core, never beside a timed invocation.

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

10. **Strata.** The jobs fall into five strata: **MONEY** (jobs 1–2), **SMALL** (3–4), **MOD** (5–6), **NEW** (7) and **CHAINS** (8–11). For each stratum, each comparison (L1 and L1-opt, each against ctl, fmpq, mpq, best library and fz; L1-opt against L1) and each measure, the report gives the counts of wins, ties, losses, over-cap and n/a results, the median ratio over the stratum's jobs, and the number of severe-memory-loss flags. The per-job verdicts and magnitudes are listed beneath.

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

Width 64. Rows L1, L1-opt, L1-null, ctl, fmpq and mpq on every job; fz where marked.

| Job | Name | Family | Parameters | fz |
| --- | --- | --- | --- | --- |
| 1 | MONEY-wide | F1 | amounts uniform in $(-10^7, 10^7)$ cents; add, sub | yes |
| 2 | MONEY-catalog | F1 | 200 prices; add, sub | yes |
| 3 | SMALL8 | F2 | $k = 8$; mul, div | |
| 4 | SMALL64 | F2 | $k = 64$; mul, div | |
| 5 | MOD-mixed | F3 | pool of 1,000; add, sub, mul, div | |
| 6 | MOD-muldiv | F3 | pool of 1,000; mul, div | |
| 7 | NEW-add | F4 | 08 P50-add construction; add | |
| 8 | CH-money | F5 | K = 32, add, sub; catalog inputs | yes |
| 9 | CH-closed | F5 | 08 G5 CH-closed | |
| 10 | CH-small | F5 | K = 8, mul, div; whole numbers below 64 | |
| 11 | CH-mixed | F5 | K = 8, four operations; whole numbers below 64 | |

The jobs span reuse from high (SMALL8, CH-closed, CH-small) through moderate (SMALL64, MOD, MONEY-catalog) to low (MONEY-wide, NEW-add); the realized figures are reported by item 12.

**Order of work.**
1. Generate the workloads, the exact references and the predicted figures.
2. Unit tests of L1 and ctl, in the test build.
3. **Pilot**: jobs 1, 3, 5, 7 and 10, every row, in the timing build, with form and r calibration and **2 rounds**, one timed invocation at a time on core 1. Its results and calibration are discarded. **Stop rule:** the harness projects the whole run as the time elapsed so far, plus the remaining work's upper estimate below (8.5 minutes: gates, counting and memory runs 2.5, calibration and rounds 6) times the larger of 1 and the pilot's wall time over **1 minute**. **If the projection exceeds 45 minutes, the run stops and is reported to the author before anything else runs.** The limit is set below the 60-minute hard stop so that a run projected to fit keeps a quarter of its time for re-runs under item 5.
4. Gates (item 3), two at a time on both cores.
5. Counting runs and memory runs (items 2 and 6), two at a time on both cores.
6. Calibration and the 12 rounds (item 4), job by job, one timed invocation at a time on core 1, with nothing else running.

**Hard stop at 60 minutes.** The whole run, from the start of step 1 to the end of the last round, re-runs under item 5 included, has a hard limit of **60 minutes** of the run's own wall time, summed over its sessions; time spent fixing a bug under review, between sessions, is not counted (08 harness R-m). When it is reached the harness stops at once, writes what it has, marked **stopped**, with the step and job, and reports to the author; no verdict is reported from a stopped run.

*Non-normative estimate,* to be confirmed by the pilot. A 64-bit gcd costs 30–60 ns.
- **One complete run of a job** (30,000 stored values): ctl 5–20 ms (08's estimate for the same size); L1 10–40 ms (two lookups per stored value); L1-opt 8–40 ms; fmpq 5–20 ms; mpq 10–40 ms; fz 2–10 ms. So r is 2 to 16, and an invocation (3 reps of 50–100 ms, and process start) takes 0.2–0.35 s.
- **One round:** 69 invocations (11 jobs × 6 rows, and fz on 3), 14–24 s. **Calibration:** 132 form invocations at r = 1 and 69 r-calibrations, about 0.5–1 minute. **Calibration and 12 rounds: 3.5–6 minutes.**
- **Gates:** about 670 full audits per job and row, each linear in N + W + S with one gcd per rational, 1–3 ms: under 1.5 minutes on two cores, with L1-opt. **Counting and memory runs:** under 1 minute, with counters read after every call. **Generation, references and predictions:** 1–3 minutes. **Unit tests and builds:** about 2 minutes. **Pilot:** 0.5–1 minute.
- **The whole run: about 9 to 17 minutes.** If every row is three times slower than estimated, about 50 minutes: below the 60-minute hard stop, though the pilot's 45-minute projection limit would then stop the run first and report it to the author.

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

## Run summary

- started 2026-10-29T08:18:29-05:00; elapsed (run's own clock) 7.6 min
- libraries: version row=fmpq gmp=6.3.0 flint=3.0.1 threads=1 form=A
- 08 copies (lib/vendor08, commit 08bbfbe): sha256 checked before the run
- unit tests unit-layer1: pass (unit: 1146103 checks, 0 failures)
- unit tests diff-layer1: pass (checksum 7b6083446efe8a13)
- unit tests unit-08: pass (unit (width 64): 3379 checks, 0 failures)
- finished 2026-10-29T08:26:11-05:00
- gates: 33 of 33 passed

## Pilot (rule, Run plan step 3; discarded)

Jobs [1, 3, 5, 7, 10], every row, 2 rounds. Pilot wall time 18.6 s (factor 1.00); elapsed at the end of the pilot 0.52 min; **projected whole run 9.0 min** (limit 45 min).

Median per-run total over the pilot's rounds (ms), with the calibrated r and form:

| job | L1 | L1-opt | L1-null | ctl | fmpq | mpq | fz |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | 2.36 (r=32) | 2.40 (r=32) | 2.36 (r=32) | 1.60 (r=32) | 1.73 (r=32 B) | 4.00 (r=16 B) | 1.38 (r=64) |
| 3 | 1.90 (r=32) | 1.77 (r=32) | 1.90 (r=32) | 1.69 (r=32) | 1.52 (r=64 A) | 4.07 (r=16 B) | - |
| 5 | 3.58 (r=16) | 3.71 (r=16) | 3.55 (r=16) | 2.41 (r=32) | 2.74 (r=32 B) | 4.70 (r=16 B) | - |
| 7 | 3.78 (r=16) | 3.52 (r=16) | 3.44 (r=16) | 2.25 (r=32) | 2.54 (r=32 A) | 3.96 (r=16 B) | - |
| 10 | 1.09 (r=64) | 1.15 (r=64) | 1.08 (r=64) | 0.71 (r=128) | 0.78 (r=128 A) | 1.68 (r=32 B) | - |

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

## Verdicts (rule items 8-10; per measure only, no overall winner)

*Limitation:* L1-opt is timed with all its adopted tricks on together; the counters show each trick's effect on the work, but no trick's effect on time is measured alone (rule, Limitations: one row with every trick).

### Strata (rule item 10)

| stratum | X | Y | measure | win | tie | loss | over cap | n/a | median ratio | severe flags |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| MONEY | L1 | best | time | 0 | 0 | 2 | 0 | 0 | 0.739 | 0 |
| MONEY | L1 | ctl | B_heap | 0 | 1 | 1 | 0 | 0 | 1.259 | 0 |
| MONEY | L1 | ctl | B_used | 0 | 1 | 1 | 0 | 0 | 1.287 | 0 |
| MONEY | L1 | ctl | gcd work | 0 | 2 | 0 | 0 | 0 | 1.000 | 0 |
| MONEY | L1 | ctl | multiplications | 0 | 2 | 0 | 0 | 0 | 1.000 | 0 |
| MONEY | L1 | ctl | time | 0 | 0 | 2 | 0 | 0 | 0.684 | 0 |
| MONEY | L1 | fmpq | B_heap | 0 | 0 | 2 | 0 | 0 | 1.374 | 0 |
| MONEY | L1 | fmpq | B_used | 0 | 1 | 1 | 0 | 0 | 1.286 | 0 |
| MONEY | L1 | fmpq | time | 0 | 0 | 2 | 0 | 0 | 0.739 | 0 |
| MONEY | L1 | fz | B_heap | 0 | 0 | 2 | 0 | 0 | 2.724 | 2 |
| MONEY | L1 | fz | B_used | 0 | 0 | 2 | 0 | 0 | 2.572 | 2 |
| MONEY | L1 | fz | time | 0 | 0 | 2 | 0 | 0 | 0.650 | 0 |
| MONEY | L1 | mpq | B_heap | 2 | 0 | 0 | 0 | 0 | 0.231 | 0 |
| MONEY | L1 | mpq | B_used | 2 | 0 | 0 | 0 | 0 | 0.385 | 0 |
| MONEY | L1 | mpq | time | 2 | 0 | 0 | 0 | 0 | 1.895 | 0 |
| MONEY | L1-opt | L1 | B_heap | 0 | 0 | 2 | 0 | 0 | 1.249 | 0 |
| MONEY | L1-opt | L1 | B_used | 0 | 0 | 2 | 0 | 0 | 1.309 | 0 |
| MONEY | L1-opt | L1 | gcd work | 0 | 2 | 0 | 0 | 0 | 0.996 | 0 |
| MONEY | L1-opt | L1 | lookups | 0 | 2 | 0 | 0 | 0 | 0.989 | 0 |
| MONEY | L1-opt | L1 | multiplications | 0 | 2 | 0 | 0 | 0 | 0.982 | 0 |
| MONEY | L1-opt | L1 | time | 0 | 2 | 0 | 0 | 0 | 0.990 | 0 |
| MONEY | L1-opt | best | time | 0 | 0 | 2 | 0 | 0 | 0.731 | 0 |
| MONEY | L1-opt | ctl | B_heap | 0 | 0 | 2 | 0 | 0 | 1.584 | 0 |
| MONEY | L1-opt | ctl | B_used | 0 | 0 | 2 | 0 | 0 | 1.696 | 1 |
| MONEY | L1-opt | ctl | gcd work | 0 | 2 | 0 | 0 | 0 | 0.996 | 0 |
| MONEY | L1-opt | ctl | multiplications | 0 | 2 | 0 | 0 | 0 | 0.982 | 0 |
| MONEY | L1-opt | ctl | time | 0 | 0 | 2 | 0 | 0 | 0.677 | 0 |
| MONEY | L1-opt | fmpq | B_heap | 0 | 0 | 2 | 0 | 0 | 1.729 | 1 |
| MONEY | L1-opt | fmpq | B_used | 0 | 0 | 2 | 0 | 0 | 1.696 | 1 |
| MONEY | L1-opt | fmpq | time | 0 | 0 | 2 | 0 | 0 | 0.731 | 0 |
| MONEY | L1-opt | fz | B_heap | 0 | 0 | 2 | 0 | 0 | 3.428 | 2 |
| MONEY | L1-opt | fz | B_used | 0 | 0 | 2 | 0 | 0 | 3.391 | 2 |
| MONEY | L1-opt | fz | time | 0 | 0 | 2 | 0 | 0 | 0.645 | 0 |
| MONEY | L1-opt | mpq | B_heap | 2 | 0 | 0 | 0 | 0 | 0.290 | 0 |
| MONEY | L1-opt | mpq | B_used | 2 | 0 | 0 | 0 | 0 | 0.507 | 0 |
| MONEY | L1-opt | mpq | time | 2 | 0 | 0 | 0 | 0 | 1.878 | 0 |
| SMALL | L1 | best | time | 0 | 0 | 2 | 0 | 0 | 0.771 | 0 |
| SMALL | L1 | ctl | B_heap | 1 | 0 | 1 | 0 | 0 | 1.017 | 0 |
| SMALL | L1 | ctl | B_used | 1 | 0 | 1 | 0 | 0 | 0.958 | 0 |
| SMALL | L1 | ctl | gcd work | 1 | 1 | 0 | 0 | 0 | 0.920 | 0 |
| SMALL | L1 | ctl | multiplications | 1 | 1 | 0 | 0 | 0 | 0.927 | 0 |
| SMALL | L1 | ctl | time | 0 | 0 | 2 | 0 | 0 | 0.787 | 0 |
| SMALL | L1 | fmpq | B_heap | 1 | 0 | 1 | 0 | 0 | 1.111 | 0 |
| SMALL | L1 | fmpq | B_used | 1 | 0 | 1 | 0 | 0 | 0.958 | 0 |
| SMALL | L1 | fmpq | time | 0 | 0 | 2 | 0 | 0 | 0.771 | 0 |
| SMALL | L1 | mpq | B_heap | 2 | 0 | 0 | 0 | 0 | 0.186 | 0 |
| SMALL | L1 | mpq | B_used | 2 | 0 | 0 | 0 | 0 | 0.261 | 0 |
| SMALL | L1 | mpq | time | 2 | 0 | 0 | 0 | 0 | 1.847 | 0 |
| SMALL | L1-opt | L1 | B_heap | 0 | 1 | 1 | 0 | 0 | 1.114 | 0 |
| SMALL | L1-opt | L1 | B_used | 0 | 1 | 1 | 0 | 0 | 1.203 | 0 |
| SMALL | L1-opt | L1 | gcd work | 1 | 1 | 0 | 0 | 0 | 0.869 | 0 |
| SMALL | L1-opt | L1 | lookups | 1 | 1 | 0 | 0 | 0 | 0.905 | 0 |
| SMALL | L1-opt | L1 | multiplications | 1 | 1 | 0 | 0 | 0 | 0.812 | 0 |
| SMALL | L1-opt | L1 | time | 1 | 1 | 0 | 0 | 0 | 1.038 | 0 |
| SMALL | L1-opt | best | time | 0 | 0 | 2 | 0 | 0 | 0.801 | 0 |
| SMALL | L1-opt | ctl | B_heap | 1 | 0 | 1 | 0 | 0 | 1.223 | 0 |
| SMALL | L1-opt | ctl | B_used | 1 | 0 | 1 | 0 | 0 | 1.235 | 0 |
| SMALL | L1-opt | ctl | gcd work | 1 | 1 | 0 | 0 | 0 | 0.806 | 0 |
| SMALL | L1-opt | ctl | multiplications | 1 | 1 | 0 | 0 | 0 | 0.761 | 0 |
| SMALL | L1-opt | ctl | time | 0 | 1 | 1 | 0 | 0 | 0.820 | 0 |
| SMALL | L1-opt | fmpq | B_heap | 1 | 0 | 1 | 0 | 0 | 1.336 | 1 |
| SMALL | L1-opt | fmpq | B_used | 1 | 0 | 1 | 0 | 0 | 1.235 | 0 |
| SMALL | L1-opt | fmpq | time | 0 | 0 | 2 | 0 | 0 | 0.801 | 0 |
| SMALL | L1-opt | mpq | B_heap | 2 | 0 | 0 | 0 | 0 | 0.224 | 0 |
| SMALL | L1-opt | mpq | B_used | 2 | 0 | 0 | 0 | 0 | 0.337 | 0 |
| SMALL | L1-opt | mpq | time | 2 | 0 | 0 | 0 | 0 | 1.929 | 0 |
| MOD | L1 | best | time | 0 | 0 | 2 | 0 | 0 | 0.760 | 0 |
| MOD | L1 | ctl | B_heap | 0 | 0 | 2 | 0 | 0 | 2.499 | 2 |
| MOD | L1 | ctl | B_used | 0 | 0 | 2 | 0 | 0 | 2.273 | 2 |
| MOD | L1 | ctl | gcd work | 0 | 2 | 0 | 0 | 0 | 0.999 | 0 |
| MOD | L1 | ctl | multiplications | 0 | 2 | 0 | 0 | 0 | 0.999 | 0 |
| MOD | L1 | ctl | time | 0 | 0 | 2 | 0 | 0 | 0.683 | 0 |
| MOD | L1 | fmpq | B_heap | 0 | 0 | 2 | 0 | 0 | 2.728 | 2 |
| MOD | L1 | fmpq | B_used | 0 | 0 | 2 | 0 | 0 | 2.272 | 2 |
| MOD | L1 | fmpq | time | 0 | 0 | 2 | 0 | 0 | 0.760 | 0 |
| MOD | L1 | mpq | B_heap | 2 | 0 | 0 | 0 | 0 | 0.458 | 0 |
| MOD | L1 | mpq | B_used | 2 | 0 | 0 | 0 | 0 | 0.626 | 0 |
| MOD | L1 | mpq | time | 2 | 0 | 0 | 0 | 0 | 1.345 | 0 |
| MOD | L1-opt | L1 | B_heap | 0 | 0 | 2 | 0 | 0 | 1.378 | 0 |
| MOD | L1-opt | L1 | B_used | 0 | 0 | 2 | 0 | 0 | 1.481 | 0 |
| MOD | L1-opt | L1 | gcd work | 0 | 2 | 0 | 0 | 0 | 1.000 | 0 |
| MOD | L1-opt | L1 | lookups | 0 | 2 | 0 | 0 | 0 | 1.000 | 0 |
| MOD | L1-opt | L1 | multiplications | 0 | 2 | 0 | 0 | 0 | 1.000 | 0 |
| MOD | L1-opt | L1 | time | 0 | 1 | 1 | 0 | 0 | 0.945 | 0 |
| MOD | L1-opt | best | time | 0 | 0 | 2 | 0 | 0 | 0.718 | 0 |
| MOD | L1-opt | ctl | B_heap | 0 | 0 | 2 | 0 | 0 | 3.444 | 2 |
| MOD | L1-opt | ctl | B_used | 0 | 0 | 2 | 0 | 0 | 3.365 | 2 |
| MOD | L1-opt | ctl | gcd work | 0 | 2 | 0 | 0 | 0 | 0.999 | 0 |
| MOD | L1-opt | ctl | multiplications | 0 | 2 | 0 | 0 | 0 | 0.999 | 0 |
| MOD | L1-opt | ctl | time | 0 | 0 | 2 | 0 | 0 | 0.645 | 0 |
| MOD | L1-opt | fmpq | B_heap | 0 | 0 | 2 | 0 | 0 | 3.760 | 2 |
| MOD | L1-opt | fmpq | B_used | 0 | 0 | 2 | 0 | 0 | 3.364 | 2 |
| MOD | L1-opt | fmpq | time | 0 | 0 | 2 | 0 | 0 | 0.718 | 0 |
| MOD | L1-opt | mpq | B_heap | 2 | 0 | 0 | 0 | 0 | 0.631 | 0 |
| MOD | L1-opt | mpq | B_used | 0 | 2 | 0 | 0 | 0 | 0.927 | 0 |
| MOD | L1-opt | mpq | time | 2 | 0 | 0 | 0 | 0 | 1.271 | 0 |
| NEW | L1 | best | time | 0 | 0 | 1 | 0 | 0 | 0.739 | 0 |
| NEW | L1 | ctl | B_heap | 0 | 0 | 1 | 0 | 0 | 2.498 | 1 |
| NEW | L1 | ctl | B_used | 0 | 0 | 1 | 0 | 0 | 2.439 | 1 |
| NEW | L1 | ctl | gcd work | 0 | 1 | 0 | 0 | 0 | 1.000 | 0 |
| NEW | L1 | ctl | multiplications | 0 | 1 | 0 | 0 | 0 | 1.000 | 0 |
| NEW | L1 | ctl | time | 0 | 0 | 1 | 0 | 0 | 0.658 | 0 |
| NEW | L1 | fmpq | B_heap | 0 | 0 | 1 | 0 | 0 | 2.727 | 1 |
| NEW | L1 | fmpq | B_used | 0 | 0 | 1 | 0 | 0 | 2.438 | 1 |
| NEW | L1 | fmpq | time | 0 | 0 | 1 | 0 | 0 | 0.739 | 0 |
| NEW | L1 | mpq | B_heap | 1 | 0 | 0 | 0 | 0 | 0.458 | 0 |
| NEW | L1 | mpq | B_used | 1 | 0 | 0 | 0 | 0 | 0.697 | 0 |
| NEW | L1 | mpq | time | 1 | 0 | 0 | 0 | 0 | 1.194 | 0 |
| NEW | L1-opt | L1 | B_heap | 0 | 0 | 1 | 0 | 0 | 1.379 | 0 |
| NEW | L1-opt | L1 | B_used | 0 | 0 | 1 | 0 | 0 | 1.448 | 0 |
| NEW | L1-opt | L1 | gcd work | 0 | 1 | 0 | 0 | 0 | 1.000 | 0 |
| NEW | L1-opt | L1 | lookups | 0 | 1 | 0 | 0 | 0 | 1.000 | 0 |
| NEW | L1-opt | L1 | multiplications | 0 | 1 | 0 | 0 | 0 | 1.000 | 0 |
| NEW | L1-opt | L1 | time | 0 | 1 | 0 | 0 | 0 | 0.954 | 0 |
| NEW | L1-opt | best | time | 0 | 0 | 1 | 0 | 0 | 0.705 | 0 |
| NEW | L1-opt | ctl | B_heap | 0 | 0 | 1 | 0 | 0 | 3.444 | 1 |
| NEW | L1-opt | ctl | B_used | 0 | 0 | 1 | 0 | 0 | 3.531 | 1 |
| NEW | L1-opt | ctl | gcd work | 0 | 1 | 0 | 0 | 0 | 1.000 | 0 |
| NEW | L1-opt | ctl | multiplications | 0 | 1 | 0 | 0 | 0 | 1.000 | 0 |
| NEW | L1-opt | ctl | time | 0 | 0 | 1 | 0 | 0 | 0.627 | 0 |
| NEW | L1-opt | fmpq | B_heap | 0 | 0 | 1 | 0 | 0 | 3.760 | 1 |
| NEW | L1-opt | fmpq | B_used | 0 | 0 | 1 | 0 | 0 | 3.530 | 1 |
| NEW | L1-opt | fmpq | time | 0 | 0 | 1 | 0 | 0 | 0.705 | 0 |
| NEW | L1-opt | mpq | B_heap | 1 | 0 | 0 | 0 | 0 | 0.631 | 0 |
| NEW | L1-opt | mpq | B_used | 0 | 1 | 0 | 0 | 0 | 1.009 | 0 |
| NEW | L1-opt | mpq | time | 1 | 0 | 0 | 0 | 0 | 1.139 | 0 |
| CHAINS | L1 | best | time | 0 | 0 | 4 | 0 | 0 | 0.730 | 0 |
| CHAINS | L1 | ctl | B_heap | 0 | 4 | 0 | 0 | 0 | 1.009 | 0 |
| CHAINS | L1 | ctl | B_used | 0 | 0 | 4 | 0 | 0 | 1.244 | 0 |
| CHAINS | L1 | ctl | gcd work | 0 | 4 | 0 | 0 | 0 | 0.999 | 0 |
| CHAINS | L1 | ctl | multiplications | 0 | 4 | 0 | 0 | 0 | 0.999 | 0 |
| CHAINS | L1 | ctl | time | 0 | 0 | 4 | 0 | 0 | 0.670 | 0 |
| CHAINS | L1 | fmpq | B_heap | 0 | 0 | 4 | 0 | 0 | 1.698 | 0 |
| CHAINS | L1 | fmpq | B_used | 0 | 0 | 4 | 0 | 0 | 1.243 | 0 |
| CHAINS | L1 | fmpq | time | 0 | 0 | 4 | 0 | 0 | 0.730 | 0 |
| CHAINS | L1 | fz | B_heap | 0 | 0 | 1 | 0 | 0 | 3.113 | 1 |
| CHAINS | L1 | fz | B_used | 0 | 0 | 1 | 0 | 0 | 2.443 | 1 |
| CHAINS | L1 | fz | time | 0 | 0 | 1 | 0 | 0 | 0.687 | 0 |
| CHAINS | L1 | mpq | B_heap | 4 | 0 | 0 | 0 | 0 | 0.286 | 0 |
| CHAINS | L1 | mpq | B_used | 4 | 0 | 0 | 0 | 0 | 0.364 | 0 |
| CHAINS | L1 | mpq | time | 4 | 0 | 0 | 0 | 0 | 1.678 | 0 |
| CHAINS | L1-opt | L1 | B_heap | 0 | 4 | 0 | 0 | 0 | 1.043 | 0 |
| CHAINS | L1-opt | L1 | B_used | 0 | 0 | 4 | 0 | 0 | 1.334 | 0 |
| CHAINS | L1-opt | L1 | gcd work | 0 | 4 | 0 | 0 | 0 | 0.988 | 0 |
| CHAINS | L1-opt | L1 | lookups | 0 | 4 | 0 | 0 | 0 | 0.989 | 0 |
| CHAINS | L1-opt | L1 | multiplications | 0 | 4 | 0 | 0 | 0 | 0.975 | 0 |
| CHAINS | L1-opt | L1 | time | 0 | 4 | 0 | 0 | 0 | 1.008 | 0 |
| CHAINS | L1-opt | best | time | 0 | 0 | 4 | 0 | 0 | 0.735 | 0 |
| CHAINS | L1-opt | ctl | B_heap | 0 | 4 | 0 | 0 | 0 | 1.052 | 0 |
| CHAINS | L1-opt | ctl | B_used | 0 | 0 | 4 | 0 | 0 | 1.666 | 0 |
| CHAINS | L1-opt | ctl | gcd work | 0 | 4 | 0 | 0 | 0 | 0.988 | 0 |
| CHAINS | L1-opt | ctl | multiplications | 0 | 4 | 0 | 0 | 0 | 0.974 | 0 |
| CHAINS | L1-opt | ctl | time | 0 | 0 | 4 | 0 | 0 | 0.675 | 0 |
| CHAINS | L1-opt | fmpq | B_heap | 0 | 0 | 4 | 0 | 0 | 1.767 | 0 |
| CHAINS | L1-opt | fmpq | B_used | 0 | 0 | 4 | 0 | 0 | 1.666 | 0 |
| CHAINS | L1-opt | fmpq | time | 0 | 0 | 4 | 0 | 0 | 0.735 | 0 |
| CHAINS | L1-opt | fz | B_heap | 0 | 0 | 1 | 0 | 0 | 3.321 | 1 |
| CHAINS | L1-opt | fz | B_used | 0 | 0 | 1 | 0 | 0 | 3.223 | 1 |
| CHAINS | L1-opt | fz | time | 0 | 0 | 1 | 0 | 0 | 0.688 | 0 |
| CHAINS | L1-opt | mpq | B_heap | 4 | 0 | 0 | 0 | 0 | 0.297 | 0 |
| CHAINS | L1-opt | mpq | B_used | 4 | 0 | 0 | 0 | 0 | 0.488 | 0 |
| CHAINS | L1-opt | mpq | time | 4 | 0 | 0 | 0 | 0 | 1.693 | 0 |

### Per job and measure, with magnitudes (rule item 8)

#### Job 1 (MONEY-wide)

Null control L1 against L1-null: ratio 1.001 [0.997, 1.005], h = 0.050

| X | Y | measure | verdict | ratio | interval / after entry | flags |
| --- | --- | --- | --- | --- | --- | --- |
| L1 | ctl | time | loss | 0.678 | [0.675, 0.681], h 0.050 |  |
| L1 | ctl | B_used | loss | 1.544 | 1.819 |  |
| L1 | ctl | B_heap | loss | 1.508 | 1.523 |  |
| L1 | ctl | gcd work | tie | 1.000 | 752008 / 752008 |  |
| L1 | ctl | multiplications | tie | 1.000 | 45126 / 45126 |  |
| L1 | fmpq | time | loss | 0.738 | [0.734, 0.740], h 0.050 |  |
| L1 | fmpq | B_used | loss | 1.543 | 1.817 |  |
| L1 | fmpq | B_heap | loss | 1.646 | 2.496 |  |
| L1 | mpq | time | win | 1.706 | [1.686, 1.729], h 0.050 |  |
| L1 | mpq | B_used | win | 0.462 | 0.606 |  |
| L1 | mpq | B_heap | win | 0.276 | 0.416 |  |
| L1 | fz | time | loss | 0.587 | [0.583, 0.588], h 0.050 | ceiling gap |
| L1 | fz | B_used | loss | 3.086 | 3.631 | severe memory loss, ceiling gap |
| L1 | fz | B_heap | loss | 3.263 | 4.990 | severe memory loss, ceiling gap |
| L1 | best | time | loss | 0.738 | [0.734, 0.740], h 0.050 | best library: fmpq |
| L1-opt | ctl | time | loss | 0.664 | [0.659, 0.668], h 0.050 |  |
| L1-opt | ctl | B_used | loss | 2.090 | 2.638 | severe memory loss |
| L1-opt | ctl | B_heap | loss | 1.956 | 2.426 |  |
| L1-opt | ctl | gcd work | tie | 0.997 | 749739 / 752008 |  |
| L1-opt | ctl | multiplications | tie | 0.983 | 44348 / 45126 |  |
| L1-opt | fmpq | time | loss | 0.722 | [0.718, 0.725], h 0.050 |  |
| L1-opt | fmpq | B_used | loss | 2.089 | 2.636 | severe memory loss |
| L1-opt | fmpq | B_heap | loss | 2.135 | 3.977 | severe memory loss |
| L1-opt | mpq | time | win | 1.670 | [1.648, 1.692], h 0.050 |  |
| L1-opt | mpq | B_used | win | 0.625 | 0.879 |  |
| L1-opt | mpq | B_heap | win | 0.358 | 0.663 |  |
| L1-opt | fz | time | loss | 0.574 | [0.571, 0.576], h 0.050 | ceiling gap |
| L1-opt | fz | B_used | loss | 4.178 | 5.267 | severe memory loss, ceiling gap |
| L1-opt | fz | B_heap | loss | 4.233 | 7.949 | severe memory loss, ceiling gap |
| L1-opt | best | time | loss | 0.722 | [0.718, 0.725], h 0.050 | best library: fmpq |
| L1-opt | L1 | time | tie | 0.979 | [0.972, 0.983], h 0.050 |  |
| L1-opt | L1 | B_used | loss | 1.354 | 1.450 |  |
| L1-opt | L1 | B_heap | loss | 1.297 | 1.593 |  |
| L1-opt | L1 | gcd work | tie | 0.997 | 749739 / 752008 |  |
| L1-opt | L1 | multiplications | tie | 0.983 | 44348 / 45126 |  |
| L1-opt | L1 | lookups | tie | 0.991 | skipped: O-1 585, O-2 291 |  |

Operation time alone (entry and read-out excluded; rule item 12, verdicts by item 8 with the job's h): L1 vs ctl loss 0.698 [0.694, 0.700]; L1 vs fmpq loss 0.741 [0.737, 0.743]; L1 vs mpq win 1.832 [1.806, 1.868]; L1 vs fz loss 0.081 [0.081, 0.082]; L1-opt vs ctl loss 0.682 [0.677, 0.685]; L1-opt vs fmpq loss 0.724 [0.719, 0.727]; L1-opt vs mpq win 1.790 [1.762, 1.824]; L1-opt vs fz loss 0.079 [0.079, 0.080]; L1-opt vs L1 tie 0.977 [0.972, 0.982]

#### Job 2 (MONEY-catalog)

Null control L1 against L1-null: ratio 1.002 [1.000, 1.004], h = 0.050

| X | Y | measure | verdict | ratio | interval / after entry | flags |
| --- | --- | --- | --- | --- | --- | --- |
| L1 | ctl | time | loss | 0.690 | [0.688, 0.694], h 0.050 |  |
| L1 | ctl | B_used | tie | 1.030 | 0.523 |  |
| L1 | ctl | B_heap | tie | 1.010 | 0.535 |  |
| L1 | ctl | gcd work | tie | 1.000 | 554668 / 554668 |  |
| L1 | ctl | multiplications | tie | 1.000 | 44937 / 44937 |  |
| L1 | fmpq | time | loss | 0.740 | [0.738, 0.741], h 0.050 |  |
| L1 | fmpq | B_used | tie | 1.030 | 0.523 |  |
| L1 | fmpq | B_heap | loss | 1.102 | 0.877 |  |
| L1 | mpq | time | win | 2.084 | [2.067, 2.110], h 0.050 |  |
| L1 | mpq | B_used | win | 0.308 | 0.174 |  |
| L1 | mpq | B_heap | win | 0.185 | 0.146 |  |
| L1 | fz | time | loss | 0.714 | [0.708, 0.721], h 0.050 | ceiling gap |
| L1 | fz | B_used | loss | 2.058 | 1.045 | severe memory loss, ceiling gap |
| L1 | fz | B_heap | loss | 2.185 | 1.753 | severe memory loss, ceiling gap |
| L1 | best | time | loss | 0.740 | [0.738, 0.741], h 0.050 | best library: fmpq |
| L1-opt | ctl | time | loss | 0.691 | [0.688, 0.694], h 0.050 |  |
| L1-opt | ctl | B_used | loss | 1.303 | 0.536 |  |
| L1-opt | ctl | B_heap | loss | 1.212 | 0.950 |  |
| L1-opt | ctl | gcd work | tie | 0.995 | 551981 / 554668 |  |
| L1-opt | ctl | multiplications | tie | 0.981 | 44064 / 44937 |  |
| L1-opt | fmpq | time | loss | 0.741 | [0.736, 0.742], h 0.050 |  |
| L1-opt | fmpq | B_used | loss | 1.303 | 0.536 |  |
| L1-opt | fmpq | B_heap | loss | 1.323 | 1.558 |  |
| L1-opt | mpq | time | win | 2.087 | [2.067, 2.113], h 0.050 |  |
| L1-opt | mpq | B_used | win | 0.389 | 0.179 |  |
| L1-opt | mpq | B_heap | win | 0.222 | 0.260 |  |
| L1-opt | fz | time | loss | 0.715 | [0.709, 0.722], h 0.050 | ceiling gap |
| L1-opt | fz | B_used | loss | 2.604 | 1.070 | severe memory loss, ceiling gap |
| L1-opt | fz | B_heap | loss | 2.623 | 3.114 | severe memory loss, ceiling gap |
| L1-opt | best | time | loss | 0.741 | [0.736, 0.742], h 0.050 | best library: fmpq |
| L1-opt | L1 | time | tie | 1.001 | [0.995, 1.003], h 0.050 |  |
| L1-opt | L1 | B_used | loss | 1.265 | 1.024 |  |
| L1-opt | L1 | B_heap | loss | 1.200 | 1.776 |  |
| L1-opt | L1 | gcd work | tie | 0.995 | 551981 / 554668 |  |
| L1-opt | L1 | multiplications | tie | 0.981 | 44064 / 44937 |  |
| L1-opt | L1 | lookups | tie | 0.987 | skipped: O-1 610, O-2 367 |  |

Operation time alone (entry and read-out excluded; rule item 12, verdicts by item 8 with the job's h): L1 vs ctl loss 0.673 [0.670, 0.677]; L1 vs fmpq loss 0.698 [0.694, 0.701]; L1 vs mpq win 2.101 [2.084, 2.139]; L1 vs fz loss 0.088 [0.088, 0.090]; L1-opt vs ctl loss 0.677 [0.675, 0.682]; L1-opt vs fmpq loss 0.703 [0.700, 0.707]; L1-opt vs mpq win 2.114 [2.099, 2.153]; L1-opt vs fz loss 0.089 [0.088, 0.091]; L1-opt vs L1 tie 1.006 [1.002, 1.015]

#### Job 3 (SMALL8)

Null control L1 against L1-null: ratio 0.999 [0.997, 1.004], h = 0.050

| X | Y | measure | verdict | ratio | interval / after entry | flags |
| --- | --- | --- | --- | --- | --- | --- |
| L1 | ctl | time | loss | 0.888 | [0.886, 0.891], h 0.050 |  |
| L1 | ctl | B_used | win | 0.514 | 0.501 |  |
| L1 | ctl | B_heap | win | 0.527 | 0.501 |  |
| L1 | ctl | gcd work | win | 0.853 | 625245 / 732913 |  |
| L1 | ctl | multiplications | win | 0.869 | 32872 / 37844 |  |
| L1 | fmpq | time | loss | 0.803 | [0.801, 0.805], h 0.050 |  |
| L1 | fmpq | B_used | win | 0.514 | 0.500 |  |
| L1 | fmpq | B_heap | win | 0.576 | 0.821 |  |
| L1 | mpq | time | win | 2.169 | [2.151, 2.185], h 0.050 |  |
| L1 | mpq | B_used | win | 0.140 | 0.167 |  |
| L1 | mpq | B_heap | win | 0.097 | 0.137 |  |
| L1 | best | time | loss | 0.803 | [0.801, 0.805], h 0.050 | best library: fmpq |
| L1-opt | ctl | time | tie | 0.954 | [0.951, 0.956], h 0.050 |  |
| L1-opt | ctl | B_used | win | 0.523 | 0.501 |  |
| L1-opt | ctl | B_heap | win | 0.491 | 0.917 |  |
| L1-opt | ctl | gcd work | win | 0.654 | 479641 / 732913 |  |
| L1-opt | ctl | multiplications | win | 0.576 | 21794 / 37844 |  |
| L1-opt | fmpq | time | loss | 0.862 | [0.860, 0.864], h 0.050 |  |
| L1-opt | fmpq | B_used | win | 0.523 | 0.501 |  |
| L1-opt | fmpq | B_heap | win | 0.536 | 1.503 |  |
| L1-opt | mpq | time | win | 2.329 | [2.307, 2.348], h 0.050 |  |
| L1-opt | mpq | B_used | win | 0.143 | 0.167 |  |
| L1-opt | mpq | B_heap | win | 0.090 | 0.250 |  |
| L1-opt | best | time | loss | 0.862 | [0.860, 0.864], h 0.050 | best library: fmpq |
| L1-opt | L1 | time | win | 1.074 | [1.071, 1.076], h 0.050 |  |
| L1-opt | L1 | B_used | tie | 1.017 | 1.001 |  |
| L1-opt | L1 | B_heap | tie | 0.931 | 1.829 |  |
| L1-opt | L1 | gcd work | win | 0.767 | 479641 / 625245 |  |
| L1-opt | L1 | multiplications | win | 0.663 | 21794 / 32872 |  |
| L1-opt | L1 | lookups | win | 0.821 | skipped: O-1 3670, O-2 7180 |  |

Operation time alone (entry and read-out excluded; rule item 12, verdicts by item 8 with the job's h): L1 vs ctl tie 0.985 [0.980, 0.988]; L1 vs fmpq loss 0.870 [0.867, 0.873]; L1 vs mpq win 2.403 [2.371, 2.426]; L1-opt vs ctl tie 1.045 [1.043, 1.048]; L1-opt vs fmpq loss 0.923 [0.922, 0.927]; L1-opt vs mpq win 2.550 [2.517, 2.576]; L1-opt vs L1 win 1.061 [1.059, 1.066]

#### Job 4 (SMALL64)

Null control L1 against L1-null: ratio 1.000 [0.988, 1.009], h = 0.050

| X | Y | measure | verdict | ratio | interval / after entry | flags |
| --- | --- | --- | --- | --- | --- | --- |
| L1 | ctl | time | loss | 0.686 | [0.681, 0.694], h 0.050 |  |
| L1 | ctl | B_used | loss | 1.402 | 0.506 |  |
| L1 | ctl | B_heap | loss | 1.508 | 0.516 |  |
| L1 | ctl | gcd work | tie | 0.987 | 956803 / 969574 |  |
| L1 | ctl | multiplications | tie | 0.985 | 39242 / 39822 |  |
| L1 | fmpq | time | loss | 0.740 | [0.735, 0.744], h 0.050 |  |
| L1 | fmpq | B_used | loss | 1.402 | 0.506 |  |
| L1 | fmpq | B_heap | loss | 1.646 | 0.845 |  |
| L1 | mpq | time | win | 1.526 | [1.505, 1.541], h 0.050 |  |
| L1 | mpq | B_used | win | 0.382 | 0.169 |  |
| L1 | mpq | B_heap | win | 0.276 | 0.141 |  |
| L1 | best | time | loss | 0.740 | [0.735, 0.744], h 0.050 | best library: fmpq |
| L1-opt | ctl | time | loss | 0.687 | [0.679, 0.694], h 0.050 |  |
| L1-opt | ctl | B_used | loss | 1.948 | 0.510 |  |
| L1-opt | ctl | B_heap | loss | 1.956 | 0.927 |  |
| L1-opt | ctl | gcd work | tie | 0.958 | 928636 / 969574 |  |
| L1-opt | ctl | multiplications | tie | 0.947 | 37703 / 39822 |  |
| L1-opt | fmpq | time | loss | 0.741 | [0.733, 0.748], h 0.050 |  |
| L1-opt | fmpq | B_used | loss | 1.948 | 0.509 |  |
| L1-opt | fmpq | B_heap | loss | 2.135 | 1.519 | severe memory loss |
| L1-opt | mpq | time | win | 1.529 | [1.505, 1.541], h 0.050 |  |
| L1-opt | mpq | B_used | win | 0.531 | 0.170 |  |
| L1-opt | mpq | B_heap | win | 0.358 | 0.253 |  |
| L1-opt | best | time | loss | 0.741 | [0.733, 0.748], h 0.050 | best library: fmpq |
| L1-opt | L1 | time | tie | 1.002 | [0.990, 1.010], h 0.050 |  |
| L1-opt | L1 | B_used | loss | 1.390 | 1.006 |  |
| L1-opt | L1 | B_heap | loss | 1.297 | 1.798 |  |
| L1-opt | L1 | gcd work | tie | 0.971 | 928636 / 956803 |  |
| L1-opt | L1 | multiplications | tie | 0.961 | 37703 / 39242 |  |
| L1-opt | L1 | lookups | tie | 0.989 | skipped: O-1 520, O-2 541 |  |

Operation time alone (entry and read-out excluded; rule item 12, verdicts by item 8 with the job's h): L1 vs ctl loss 0.692 [0.687, 0.702]; L1 vs fmpq loss 0.732 [0.728, 0.739]; L1 vs mpq win 1.510 [1.489, 1.532]; L1-opt vs ctl loss 0.679 [0.672, 0.686]; L1-opt vs fmpq loss 0.718 [0.711, 0.727]; L1-opt vs mpq win 1.482 [1.458, 1.498]; L1-opt vs L1 tie 0.981 [0.970, 0.988]

#### Job 5 (MOD-mixed)

Null control L1 against L1-null: ratio 0.997 [0.990, 1.003], h = 0.050

| X | Y | measure | verdict | ratio | interval / after entry | flags |
| --- | --- | --- | --- | --- | --- | --- |
| L1 | ctl | time | loss | 0.676 | [0.671, 0.682], h 0.050 |  |
| L1 | ctl | B_used | loss | 2.273 | 0.601 | severe memory loss |
| L1 | ctl | B_heap | loss | 2.499 | 0.585 | severe memory loss |
| L1 | ctl | gcd work | tie | 1.000 | 1237360 / 1237893 |  |
| L1 | ctl | multiplications | tie | 1.000 | 49952 / 49972 |  |
| L1 | fmpq | time | loss | 0.762 | [0.758, 0.766], h 0.050 |  |
| L1 | fmpq | B_used | loss | 2.272 | 0.601 | severe memory loss |
| L1 | fmpq | B_heap | loss | 2.728 | 0.959 | severe memory loss |
| L1 | mpq | time | win | 1.310 | [1.284, 1.340], h 0.050 |  |
| L1 | mpq | B_used | win | 0.632 | 0.200 |  |
| L1 | mpq | B_heap | win | 0.458 | 0.160 |  |
| L1 | best | time | loss | 0.762 | [0.758, 0.766], h 0.050 | best library: fmpq |
| L1-opt | ctl | time | loss | 0.646 | [0.639, 0.651], h 0.050 |  |
| L1-opt | ctl | B_used | loss | 3.365 | 0.652 | severe memory loss |
| L1-opt | ctl | B_heap | loss | 3.444 | 1.021 | severe memory loss |
| L1-opt | ctl | gcd work | tie | 1.000 | 1237360 / 1237893 |  |
| L1-opt | ctl | multiplications | tie | 1.000 | 49952 / 49972 |  |
| L1-opt | fmpq | time | loss | 0.729 | [0.722, 0.733], h 0.050 |  |
| L1-opt | fmpq | B_used | loss | 3.364 | 0.652 | severe memory loss |
| L1-opt | fmpq | B_heap | loss | 3.760 | 1.673 | severe memory loss |
| L1-opt | mpq | time | win | 1.252 | [1.233, 1.273], h 0.050 |  |
| L1-opt | mpq | B_used | tie | 0.936 | 0.217 |  |
| L1-opt | mpq | B_heap | win | 0.631 | 0.279 |  |
| L1-opt | best | time | loss | 0.729 | [0.722, 0.733], h 0.050 | best library: fmpq |
| L1-opt | L1 | time | tie | 0.956 | [0.946, 0.964], h 0.050 |  |
| L1-opt | L1 | B_used | loss | 1.481 | 1.085 |  |
| L1-opt | L1 | B_heap | loss | 1.378 | 1.745 |  |
| L1-opt | L1 | gcd work | tie | 1.000 | 1237360 / 1237360 |  |
| L1-opt | L1 | multiplications | tie | 1.000 | 49952 / 49952 |  |
| L1-opt | L1 | lookups | tie | 1.000 | skipped: O-1 17, O-2 0 |  |

Operation time alone (entry and read-out excluded; rule item 12, verdicts by item 8 with the job's h): L1 vs ctl loss 0.676 [0.671, 0.682]; L1 vs fmpq loss 0.757 [0.754, 0.763]; L1 vs mpq win 1.281 [1.248, 1.314]; L1-opt vs ctl loss 0.629 [0.621, 0.634]; L1-opt vs fmpq loss 0.705 [0.698, 0.710]; L1-opt vs mpq win 1.192 [1.164, 1.215]; L1-opt vs L1 loss 0.931 [0.920, 0.939]

#### Job 6 (MOD-muldiv)

Null control L1 against L1-null: ratio 0.999 [0.995, 1.008], h = 0.050

| X | Y | measure | verdict | ratio | interval / after entry | flags |
| --- | --- | --- | --- | --- | --- | --- |
| L1 | ctl | time | loss | 0.690 | [0.686, 0.695], h 0.050 |  |
| L1 | ctl | B_used | loss | 2.272 | 0.601 | severe memory loss |
| L1 | ctl | B_heap | loss | 2.500 | 0.586 | severe memory loss |
| L1 | ctl | gcd work | tie | 0.999 | 1312147 / 1312976 |  |
| L1 | ctl | multiplications | tie | 0.999 | 39950 / 39982 |  |
| L1 | fmpq | time | loss | 0.758 | [0.755, 0.762], h 0.050 |  |
| L1 | fmpq | B_used | loss | 2.272 | 0.601 | severe memory loss |
| L1 | fmpq | B_heap | loss | 2.729 | 0.960 | severe memory loss |
| L1 | mpq | time | win | 1.380 | [1.376, 1.392], h 0.050 |  |
| L1 | mpq | B_used | win | 0.620 | 0.200 |  |
| L1 | mpq | B_heap | win | 0.458 | 0.160 |  |
| L1 | best | time | loss | 0.758 | [0.755, 0.762], h 0.050 | best library: fmpq |
| L1-opt | ctl | time | loss | 0.645 | [0.639, 0.650], h 0.050 |  |
| L1-opt | ctl | B_used | loss | 3.365 | 0.652 | severe memory loss |
| L1-opt | ctl | B_heap | loss | 3.444 | 1.021 | severe memory loss |
| L1-opt | ctl | gcd work | tie | 0.999 | 1312147 / 1312976 |  |
| L1-opt | ctl | multiplications | tie | 0.999 | 39950 / 39982 |  |
| L1-opt | fmpq | time | loss | 0.708 | [0.702, 0.714], h 0.050 |  |
| L1-opt | fmpq | B_used | loss | 3.364 | 0.652 | severe memory loss |
| L1-opt | fmpq | B_heap | loss | 3.760 | 1.673 | severe memory loss |
| L1-opt | mpq | time | win | 1.289 | [1.278, 1.303], h 0.050 |  |
| L1-opt | mpq | B_used | tie | 0.918 | 0.217 |  |
| L1-opt | mpq | B_heap | win | 0.631 | 0.279 |  |
| L1-opt | best | time | loss | 0.708 | [0.702, 0.714], h 0.050 | best library: fmpq |
| L1-opt | L1 | time | loss | 0.934 | [0.926, 0.940], h 0.050 |  |
| L1-opt | L1 | B_used | loss | 1.481 | 1.085 |  |
| L1-opt | L1 | B_heap | loss | 1.378 | 1.742 |  |
| L1-opt | L1 | gcd work | tie | 1.000 | 1312147 / 1312147 |  |
| L1-opt | L1 | multiplications | tie | 1.000 | 39950 / 39950 |  |
| L1-opt | L1 | lookups | tie | 1.000 | skipped: O-1 34, O-2 0 |  |

Operation time alone (entry and read-out excluded; rule item 12, verdicts by item 8 with the job's h): L1 vs ctl loss 0.691 [0.687, 0.695]; L1 vs fmpq loss 0.751 [0.748, 0.755]; L1 vs mpq win 1.362 [1.357, 1.374]; L1-opt vs ctl loss 0.627 [0.621, 0.633]; L1-opt vs fmpq loss 0.682 [0.675, 0.688]; L1-opt vs mpq win 1.237 [1.224, 1.251]; L1-opt vs L1 loss 0.908 [0.899, 0.916]

#### Job 7 (NEW-add)

Null control L1 against L1-null: ratio 0.996 [0.993, 1.003], h = 0.050

| X | Y | measure | verdict | ratio | interval / after entry | flags |
| --- | --- | --- | --- | --- | --- | --- |
| L1 | ctl | time | loss | 0.658 | [0.656, 0.663], h 0.050 |  |
| L1 | ctl | B_used | loss | 2.439 | 1.866 | severe memory loss |
| L1 | ctl | B_heap | loss | 2.498 | 1.519 | severe memory loss |
| L1 | ctl | gcd work | tie | 1.000 | 1224647 / 1224647 |  |
| L1 | ctl | multiplications | tie | 1.000 | 59985 / 59985 |  |
| L1 | fmpq | time | loss | 0.739 | [0.736, 0.746], h 0.050 |  |
| L1 | fmpq | B_used | loss | 2.438 | 1.864 | severe memory loss |
| L1 | fmpq | B_heap | loss | 2.727 | 2.490 | severe memory loss |
| L1 | mpq | time | win | 1.194 | [1.160, 1.208], h 0.050 |  |
| L1 | mpq | B_used | win | 0.697 | 0.622 |  |
| L1 | mpq | B_heap | win | 0.458 | 0.415 |  |
| L1 | best | time | loss | 0.739 | [0.736, 0.746], h 0.050 | best library: fmpq |
| L1-opt | ctl | time | loss | 0.627 | [0.626, 0.631], h 0.050 |  |
| L1-opt | ctl | B_used | loss | 3.531 | 2.685 | severe memory loss |
| L1-opt | ctl | B_heap | loss | 3.444 | 2.426 | severe memory loss |
| L1-opt | ctl | gcd work | tie | 1.000 | 1224647 / 1224647 |  |
| L1-opt | ctl | multiplications | tie | 1.000 | 59985 / 59985 |  |
| L1-opt | fmpq | time | loss | 0.705 | [0.702, 0.710], h 0.050 |  |
| L1-opt | fmpq | B_used | loss | 3.530 | 2.683 | severe memory loss |
| L1-opt | fmpq | B_heap | loss | 3.760 | 3.977 | severe memory loss |
| L1-opt | mpq | time | win | 1.139 | [1.107, 1.152], h 0.050 |  |
| L1-opt | mpq | B_used | tie | 1.009 | 0.895 |  |
| L1-opt | mpq | B_heap | win | 0.631 | 0.663 |  |
| L1-opt | best | time | loss | 0.705 | [0.702, 0.710], h 0.050 | best library: fmpq |
| L1-opt | L1 | time | tie | 0.954 | [0.947, 0.960], h 0.050 |  |
| L1-opt | L1 | B_used | loss | 1.448 | 1.439 |  |
| L1-opt | L1 | B_heap | loss | 1.379 | 1.597 |  |
| L1-opt | L1 | gcd work | tie | 1.000 | 1224647 / 1224647 |  |
| L1-opt | L1 | multiplications | tie | 1.000 | 59985 / 59985 |  |
| L1-opt | L1 | lookups | tie | 1.000 | skipped: O-1 4, O-2 0 |  |

Operation time alone (entry and read-out excluded; rule item 12, verdicts by item 8 with the job's h): L1 vs ctl loss 0.672 [0.670, 0.676]; L1 vs fmpq loss 0.748 [0.744, 0.756]; L1 vs mpq win 1.167 [1.123, 1.182]; L1-opt vs ctl loss 0.624 [0.621, 0.627]; L1-opt vs fmpq loss 0.695 [0.692, 0.700]; L1-opt vs mpq tie 1.084 [1.044, 1.097]; L1-opt vs L1 loss 0.929 [0.921, 0.936]

#### Job 8 (CH-money)

Null control L1 against L1-null: ratio 0.998 [0.995, 1.002], h = 0.050

| X | Y | measure | verdict | ratio | interval / after entry | flags |
| --- | --- | --- | --- | --- | --- | --- |
| L1 | ctl | time | loss | 0.687 | [0.686, 0.689], h 0.050 |  |
| L1 | ctl | B_used | loss | 1.222 | 0.523 |  |
| L1 | ctl | B_heap | tie | 1.006 | 0.539 |  |
| L1 | ctl | gcd work | tie | 1.000 | 835655 / 835655 |  |
| L1 | ctl | multiplications | tie | 1.000 | 72963 / 72963 |  |
| L1 | fmpq | time | loss | 0.728 | [0.726, 0.732], h 0.050 | chain: every result kept |
| L1 | fmpq | B_used | loss | 1.222 | 0.523 | chain: every result kept |
| L1 | fmpq | B_heap | loss | 1.566 | 0.884 | chain: every result kept |
| L1 | mpq | time | win | 1.940 | [1.929, 1.959], h 0.050 | chain: every result kept |
| L1 | mpq | B_used | win | 0.360 | 0.174 | chain: every result kept |
| L1 | mpq | B_heap | win | 0.262 | 0.147 | chain: every result kept |
| L1 | fz | time | loss | 0.687 | [0.686, 0.690], h 0.050 | ceiling gap |
| L1 | fz | B_used | loss | 2.443 | 1.045 | severe memory loss, ceiling gap |
| L1 | fz | B_heap | loss | 3.113 | 1.767 | severe memory loss, ceiling gap |
| L1 | best | time | loss | 0.728 | [0.726, 0.732], h 0.050 | chain: every result kept, best library: fmpq |
| L1-opt | ctl | time | loss | 0.687 | [0.686, 0.689], h 0.050 |  |
| L1-opt | ctl | B_used | loss | 1.612 | 0.536 |  |
| L1-opt | ctl | B_heap | tie | 1.073 | 1.317 |  |
| L1-opt | ctl | gcd work | tie | 0.994 | 831019 / 835655 |  |
| L1-opt | ctl | multiplications | tie | 0.979 | 71409 / 72963 |  |
| L1-opt | fmpq | time | loss | 0.729 | [0.726, 0.731], h 0.050 | chain: every result kept |
| L1-opt | fmpq | B_used | loss | 1.612 | 0.536 | chain: every result kept |
| L1-opt | fmpq | B_heap | loss | 1.671 | 2.158 | chain: every result kept |
| L1-opt | mpq | time | win | 1.942 | [1.929, 1.960], h 0.050 | chain: every result kept |
| L1-opt | mpq | B_used | win | 0.474 | 0.179 | chain: every result kept |
| L1-opt | mpq | B_heap | win | 0.280 | 0.360 | chain: every result kept |
| L1-opt | fz | time | loss | 0.688 | [0.685, 0.690], h 0.050 | ceiling gap |
| L1-opt | fz | B_used | loss | 3.223 | 1.070 | severe memory loss, ceiling gap |
| L1-opt | fz | B_heap | loss | 3.321 | 4.313 | severe memory loss, ceiling gap |
| L1-opt | best | time | loss | 0.729 | [0.726, 0.731], h 0.050 | chain: every result kept, best library: fmpq |
| L1-opt | L1 | time | tie | 1.001 | [0.997, 1.002], h 0.050 |  |
| L1-opt | L1 | B_used | loss | 1.319 | 1.024 |  |
| L1-opt | L1 | B_heap | tie | 1.067 | 2.442 |  |
| L1-opt | L1 | gcd work | tie | 0.994 | 831019 / 835655 |  |
| L1-opt | L1 | multiplications | tie | 0.979 | 71409 / 72963 |  |
| L1-opt | L1 | lookups | tie | 0.988 | skipped: O-1 907, O-2 467 |  |

Operation time alone (entry and read-out excluded; rule item 12, verdicts by item 8 with the job's h): L1 vs ctl loss 0.677 [0.676, 0.679]; L1 vs fmpq loss 0.697 [0.694, 0.699]; L1 vs mpq win 1.908 [1.894, 1.930]; L1 vs fz loss 0.088 [0.087, 0.089]; L1-opt vs ctl loss 0.679 [0.677, 0.681]; L1-opt vs fmpq loss 0.698 [0.696, 0.701]; L1-opt vs mpq win 1.912 [1.897, 1.932]; L1-opt vs fz loss 0.088 [0.088, 0.089]; L1-opt vs L1 tie 1.002 [0.999, 1.003]

*Limitation:* on this chain job every result is kept as a new element, which is not the libraries' normal use (spec §10 item 3; rule, Limitations).

#### Job 9 (CH-closed)

Null control L1 against L1-null: ratio 1.005 [0.997, 1.010], h = 0.050

| X | Y | measure | verdict | ratio | interval / after entry | flags |
| --- | --- | --- | --- | --- | --- | --- |
| L1 | ctl | time | loss | 0.707 | [0.702, 0.711], h 0.050 |  |
| L1 | ctl | B_used | loss | 1.228 | 0.868 |  |
| L1 | ctl | B_heap | tie | 1.006 | 0.772 |  |
| L1 | ctl | gcd work | tie | 1.000 | 1208899 / 1208924 |  |
| L1 | ctl | multiplications | tie | 1.000 | 95529 / 95532 |  |
| L1 | fmpq | time | loss | 0.807 | [0.801, 0.811], h 0.050 | chain: every result kept |
| L1 | fmpq | B_used | loss | 1.228 | 0.868 | chain: every result kept |
| L1 | fmpq | B_heap | loss | 1.566 | 1.266 | chain: every result kept |
| L1 | mpq | time | win | 1.789 | [1.771, 1.828], h 0.050 | chain: every result kept |
| L1 | mpq | B_used | win | 0.363 | 0.290 | chain: every result kept |
| L1 | mpq | B_heap | win | 0.262 | 0.211 | chain: every result kept |
| L1 | best | time | loss | 0.807 | [0.801, 0.811], h 0.050 | chain: every result kept, best library: fmpq |
| L1-opt | ctl | time | loss | 0.720 | [0.715, 0.725], h 0.050 |  |
| L1-opt | ctl | B_used | loss | 1.618 | 1.073 |  |
| L1-opt | ctl | B_heap | tie | 1.073 | 1.668 |  |
| L1-opt | ctl | gcd work | tie | 0.998 | 1206761 / 1208924 |  |
| L1-opt | ctl | multiplications | tie | 0.987 | 94287 / 95532 |  |
| L1-opt | fmpq | time | loss | 0.822 | [0.815, 0.829], h 0.050 | chain: every result kept |
| L1-opt | fmpq | B_used | loss | 1.618 | 1.072 | chain: every result kept |
| L1-opt | fmpq | B_heap | loss | 1.671 | 2.734 | chain: every result kept |
| L1-opt | mpq | time | win | 1.822 | [1.805, 1.861], h 0.050 | chain: every result kept |
| L1-opt | mpq | B_used | win | 0.478 | 0.358 | chain: every result kept |
| L1-opt | mpq | B_heap | win | 0.280 | 0.456 | chain: every result kept |
| L1-opt | best | time | loss | 0.822 | [0.815, 0.829], h 0.050 | chain: every result kept, best library: fmpq |
| L1-opt | L1 | time | tie | 1.019 | [1.013, 1.026], h 0.050 |  |
| L1-opt | L1 | B_used | loss | 1.318 | 1.236 |  |
| L1-opt | L1 | B_heap | tie | 1.067 | 2.160 |  |
| L1-opt | L1 | gcd work | tie | 0.998 | 1206761 / 1208899 |  |
| L1-opt | L1 | multiplications | tie | 0.987 | 94287 / 95529 |  |
| L1-opt | L1 | lookups | tie | 0.997 | skipped: O-1 7, O-2 368 |  |

Operation time alone (entry and read-out excluded; rule item 12, verdicts by item 8 with the job's h): L1 vs ctl loss 0.711 [0.708, 0.717]; L1 vs fmpq loss 0.802 [0.798, 0.807]; L1 vs mpq win 1.775 [1.761, 1.820]; L1-opt vs ctl loss 0.721 [0.715, 0.726]; L1-opt vs fmpq loss 0.812 [0.804, 0.819]; L1-opt vs mpq win 1.798 [1.777, 1.842]; L1-opt vs L1 tie 1.013 [1.004, 1.018]

*Limitation:* on this chain job every result is kept as a new element, which is not the libraries' normal use (spec §10 item 3; rule, Limitations).

#### Job 10 (CH-small)

Null control L1 against L1-null: ratio 0.999 [0.995, 1.004], h = 0.050

| X | Y | measure | verdict | ratio | interval / after entry | flags |
| --- | --- | --- | --- | --- | --- | --- |
| L1 | ctl | time | loss | 0.650 | [0.647, 0.653], h 0.050 |  |
| L1 | ctl | B_used | loss | 1.259 | 0.506 |  |
| L1 | ctl | B_heap | tie | 1.012 | 0.516 |  |
| L1 | ctl | gcd work | tie | 0.998 | 300419 / 300969 |  |
| L1 | ctl | multiplications | tie | 0.996 | 15912 / 15972 |  |
| L1 | fmpq | time | loss | 0.712 | [0.708, 0.714], h 0.050 | chain: every result kept |
| L1 | fmpq | B_used | loss | 1.259 | 0.506 | chain: every result kept |
| L1 | fmpq | B_heap | loss | 1.830 | 0.845 | chain: every result kept |
| L1 | mpq | time | win | 1.568 | [1.561, 1.589], h 0.050 | chain: every result kept |
| L1 | mpq | B_used | win | 0.366 | 0.169 | chain: every result kept |
| L1 | mpq | B_heap | win | 0.309 | 0.141 | chain: every result kept |
| L1 | best | time | loss | 0.712 | [0.708, 0.714], h 0.050 | chain: every result kept, best library: fmpq |
| L1-opt | ctl | time | loss | 0.648 | [0.646, 0.651], h 0.050 |  |
| L1-opt | ctl | B_used | loss | 1.714 | 0.510 |  |
| L1-opt | ctl | B_heap | tie | 1.030 | 0.561 |  |
| L1-opt | ctl | gcd work | tie | 0.976 | 293763 / 300969 |  |
| L1-opt | ctl | multiplications | tie | 0.966 | 15429 / 15972 |  |
| L1-opt | fmpq | time | loss | 0.710 | [0.708, 0.712], h 0.050 | chain: every result kept |
| L1-opt | fmpq | B_used | loss | 1.713 | 0.509 | chain: every result kept |
| L1-opt | fmpq | B_heap | loss | 1.864 | 0.919 | chain: every result kept |
| L1-opt | mpq | time | win | 1.564 | [1.555, 1.590], h 0.050 | chain: every result kept |
| L1-opt | mpq | B_used | win | 0.498 | 0.170 | chain: every result kept |
| L1-opt | mpq | B_heap | win | 0.314 | 0.153 | chain: every result kept |
| L1-opt | best | time | loss | 0.710 | [0.708, 0.712], h 0.050 | chain: every result kept, best library: fmpq |
| L1-opt | L1 | time | tie | 0.998 | [0.993, 1.001], h 0.050 |  |
| L1-opt | L1 | B_used | loss | 1.361 | 1.006 |  |
| L1-opt | L1 | B_heap | tie | 1.018 | 1.088 |  |
| L1-opt | L1 | gcd work | tie | 0.978 | 293763 / 300419 |  |
| L1-opt | L1 | multiplications | tie | 0.970 | 15429 / 15912 |  |
| L1-opt | L1 | lookups | tie | 0.989 | skipped: O-1 68, O-2 502 |  |

Operation time alone (entry and read-out excluded; rule item 12, verdicts by item 8 with the job's h): L1 vs ctl loss 0.618 [0.616, 0.622]; L1 vs fmpq loss 0.659 [0.655, 0.661]; L1 vs mpq win 1.373 [1.362, 1.393]; L1-opt vs ctl loss 0.604 [0.603, 0.606]; L1-opt vs fmpq loss 0.643 [0.641, 0.646]; L1-opt vs mpq win 1.340 [1.328, 1.365]; L1-opt vs L1 tie 0.976 [0.973, 0.982]

*Limitation:* on this chain job every result is kept as a new element, which is not the libraries' normal use (spec §10 item 3; rule, Limitations).

#### Job 11 (CH-mixed)

Null control L1 against L1-null: ratio 0.998 [0.991, 1.002], h = 0.050

| X | Y | measure | verdict | ratio | interval / after entry | flags |
| --- | --- | --- | --- | --- | --- | --- |
| L1 | ctl | time | loss | 0.654 | [0.649, 0.657], h 0.050 |  |
| L1 | ctl | B_used | loss | 1.307 | 0.506 |  |
| L1 | ctl | B_heap | tie | 1.012 | 0.516 |  |
| L1 | ctl | gcd work | tie | 0.999 | 299909 / 300285 |  |
| L1 | ctl | multiplications | tie | 0.997 | 19903 / 19957 |  |
| L1 | fmpq | time | loss | 0.731 | [0.726, 0.735], h 0.050 | chain: every result kept |
| L1 | fmpq | B_used | loss | 1.307 | 0.506 | chain: every result kept |
| L1 | fmpq | B_heap | loss | 1.830 | 0.845 | chain: every result kept |
| L1 | mpq | time | win | 1.527 | [1.514, 1.539], h 0.050 | chain: every result kept |
| L1 | mpq | B_used | win | 0.385 | 0.169 | chain: every result kept |
| L1 | mpq | B_heap | win | 0.309 | 0.141 | chain: every result kept |
| L1 | best | time | loss | 0.731 | [0.726, 0.735], h 0.050 | chain: every result kept, best library: fmpq |
| L1-opt | ctl | time | loss | 0.663 | [0.661, 0.667], h 0.050 |  |
| L1-opt | ctl | B_used | loss | 1.762 | 0.510 |  |
| L1-opt | ctl | B_heap | tie | 1.030 | 0.561 |  |
| L1-opt | ctl | gcd work | tie | 0.981 | 294527 / 300285 |  |
| L1-opt | ctl | multiplications | tie | 0.968 | 19327 / 19957 |  |
| L1-opt | fmpq | time | loss | 0.742 | [0.739, 0.744], h 0.050 | chain: every result kept |
| L1-opt | fmpq | B_used | loss | 1.762 | 0.509 | chain: every result kept |
| L1-opt | fmpq | B_heap | loss | 1.864 | 0.919 | chain: every result kept |
| L1-opt | mpq | time | win | 1.549 | [1.538, 1.563], h 0.050 | chain: every result kept |
| L1-opt | mpq | B_used | win | 0.519 | 0.170 | chain: every result kept |
| L1-opt | mpq | B_heap | win | 0.314 | 0.153 | chain: every result kept |
| L1-opt | best | time | loss | 0.742 | [0.739, 0.744], h 0.050 | chain: every result kept, best library: fmpq |
| L1-opt | L1 | time | tie | 1.014 | [1.010, 1.021], h 0.050 |  |
| L1-opt | L1 | B_used | loss | 1.348 | 1.006 |  |
| L1-opt | L1 | B_heap | tie | 1.018 | 1.088 |  |
| L1-opt | L1 | gcd work | tie | 0.982 | 294527 / 299909 |  |
| L1-opt | L1 | multiplications | tie | 0.971 | 19327 / 19903 |  |
| L1-opt | L1 | lookups | tie | 0.989 | skipped: O-1 41, O-2 512 |  |

Operation time alone (entry and read-out excluded; rule item 12, verdicts by item 8 with the job's h): L1 vs ctl loss 0.626 [0.622, 0.629]; L1 vs fmpq loss 0.687 [0.683, 0.690]; L1 vs mpq win 1.319 [1.305, 1.332]; L1-opt vs ctl loss 0.624 [0.622, 0.628]; L1-opt vs fmpq loss 0.684 [0.682, 0.686]; L1-opt vs mpq win 1.315 [1.302, 1.328]; L1-opt vs L1 tie 0.996 [0.992, 1.003]

*Limitation:* on this chain job every result is kept as a new element, which is not the libraries' normal use (spec §10 item 3; rule, Limitations).

## Gcd work against time (rule item 12)

A job where the row removes gcd work or multiplications (verdict win) but has no time win against the same row is a **candidate for a wider-number test**, where both cost more.

| job | X | Y | gcd work | multiplications | time | candidate |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | L1 | ctl | tie 1.000 | tie 1.000 | loss 0.678 | no |
| 1 | L1-opt | ctl | tie 0.997 | tie 0.983 | loss 0.664 | no |
| 1 | L1-opt | L1 | tie 0.997 | tie 0.983 | tie 0.979 | no |
| 2 | L1 | ctl | tie 1.000 | tie 1.000 | loss 0.690 | no |
| 2 | L1-opt | ctl | tie 0.995 | tie 0.981 | loss 0.691 | no |
| 2 | L1-opt | L1 | tie 0.995 | tie 0.981 | tie 1.001 | no |
| 3 | L1 | ctl | win 0.853 | win 0.869 | loss 0.888 | yes |
| 3 | L1-opt | ctl | win 0.654 | win 0.576 | tie 0.954 | yes |
| 3 | L1-opt | L1 | win 0.767 | win 0.663 | win 1.074 | no |
| 4 | L1 | ctl | tie 0.987 | tie 0.985 | loss 0.686 | no |
| 4 | L1-opt | ctl | tie 0.958 | tie 0.947 | loss 0.687 | no |
| 4 | L1-opt | L1 | tie 0.971 | tie 0.961 | tie 1.002 | no |
| 5 | L1 | ctl | tie 1.000 | tie 1.000 | loss 0.676 | no |
| 5 | L1-opt | ctl | tie 1.000 | tie 1.000 | loss 0.646 | no |
| 5 | L1-opt | L1 | tie 1.000 | tie 1.000 | tie 0.956 | no |
| 6 | L1 | ctl | tie 0.999 | tie 0.999 | loss 0.690 | no |
| 6 | L1-opt | ctl | tie 0.999 | tie 0.999 | loss 0.645 | no |
| 6 | L1-opt | L1 | tie 1.000 | tie 1.000 | loss 0.934 | no |
| 7 | L1 | ctl | tie 1.000 | tie 1.000 | loss 0.658 | no |
| 7 | L1-opt | ctl | tie 1.000 | tie 1.000 | loss 0.627 | no |
| 7 | L1-opt | L1 | tie 1.000 | tie 1.000 | tie 0.954 | no |
| 8 | L1 | ctl | tie 1.000 | tie 1.000 | loss 0.687 | no |
| 8 | L1-opt | ctl | tie 0.994 | tie 0.979 | loss 0.687 | no |
| 8 | L1-opt | L1 | tie 0.994 | tie 0.979 | tie 1.001 | no |
| 9 | L1 | ctl | tie 1.000 | tie 1.000 | loss 0.707 | no |
| 9 | L1-opt | ctl | tie 0.998 | tie 0.987 | loss 0.720 | no |
| 9 | L1-opt | L1 | tie 0.998 | tie 0.987 | tie 1.019 | no |
| 10 | L1 | ctl | tie 0.998 | tie 0.996 | loss 0.650 | no |
| 10 | L1-opt | ctl | tie 0.976 | tie 0.966 | loss 0.648 | no |
| 10 | L1-opt | L1 | tie 0.978 | tie 0.970 | tie 0.998 | no |
| 11 | L1 | ctl | tie 0.999 | tie 0.997 | loss 0.654 | no |
| 11 | L1-opt | ctl | tie 0.981 | tie 0.968 | loss 0.663 | no |
| 11 | L1-opt | L1 | tie 0.982 | tie 0.971 | tie 1.014 | no |

Candidates: job 3 (L1 against ctl); job 3 (L1-opt against ctl).

## Also reported (rule item 12)

### Memory against reuse (jobs by W/N at the end; bytes per stored rational, end)

| job | W/N | L1 used / heap | L1-opt used / heap | ctl used / heap | fmpq used / heap | mpq used / heap | fz used / heap | L1 formula check |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 3 | 0.012 | 8.2 / 9.3 | 8.4 / 8.7 | 16.0 / 17.6 | 16.0 / 16.1 | 58.7 / 96.1 | - | ok |
| 2 | 0.513 | 16.5 / 17.8 | 20.8 / 21.3 | 16.0 / 17.6 | 16.0 / 16.1 | 53.5 / 96.1 | 8.0 / 8.1 | ok |
| 10 | 0.608 | 20.1 / 29.7 | 27.4 / 30.2 | 16.0 / 29.4 | 16.0 / 16.2 | 55.1 / 96.2 | - | ok |
| 8 | 0.664 | 19.6 / 25.2 | 25.8 / 26.9 | 16.0 / 25.1 | 16.0 / 16.1 | 54.4 / 96.1 | 8.0 / 8.1 | ok |
| 9 | 0.676 | 19.6 / 25.2 | 25.9 / 26.9 | 16.0 / 25.1 | 16.0 / 16.1 | 54.2 / 96.1 | - | ok |
| 11 | 0.705 | 20.9 / 29.7 | 28.2 / 30.2 | 16.0 / 29.4 | 16.0 / 16.2 | 54.3 / 96.2 | - | ok |
| 4 | 0.712 | 22.4 / 26.6 | 31.2 / 34.5 | 16.0 / 17.6 | 16.0 / 16.1 | 58.7 / 96.1 | - | ok |
| 1 | 0.996 | 24.7 / 26.6 | 33.4 / 34.5 | 16.0 / 17.6 | 16.0 / 16.1 | 53.5 / 96.1 | 8.0 / 8.1 | ok |
| 6 | 1.360 | 36.4 / 44.0 | 53.8 / 60.7 | 16.0 / 17.6 | 16.0 / 16.1 | 58.7 / 96.1 | - | ok |
| 5 | 1.362 | 36.4 / 44.0 | 53.8 / 60.7 | 16.0 / 17.6 | 16.0 / 16.1 | 57.5 / 96.1 | - | ok |
| 7 | 1.693 | 39.0 / 44.0 | 56.5 / 60.7 | 16.0 / 17.6 | 16.0 / 16.1 | 56.0 / 96.1 | - | ok |

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

- job 1: entry share L1 0.258; L1-opt 0.260; ctl 0.239; fmpq 0.233; mpq 0.154; fz 0.052; L1 store share of gcd work 0 (by construction), of lookups 1.000; L1-opt store share of gcd work 0 (by construction), of lookups 1.000
- job 2: entry share L1 0.201; L1-opt 0.208; ctl 0.220; fmpq 0.216; mpq 0.148; fz 0.051; L1 store share of gcd work 0 (by construction), of lookups 1.000; L1-opt store share of gcd work 0 (by construction), of lookups 1.000
- job 3: entry share L1 0.249; L1-opt 0.245; ctl 0.184; fmpq 0.169; mpq 0.133; L1 store share of gcd work 0 (by construction), of lookups 1.000; L1-opt store share of gcd work 0 (by construction), of lookups 1.000
- job 4: entry share L1 0.165; L1-opt 0.151; ctl 0.160; fmpq 0.155; mpq 0.126; L1 store share of gcd work 0 (by construction), of lookups 1.000; L1-opt store share of gcd work 0 (by construction), of lookups 1.000
- job 5: entry share L1 0.170; L1-opt 0.151; ctl 0.173; fmpq 0.162; mpq 0.140; L1 store share of gcd work 0 (by construction), of lookups 1.000; L1-opt store share of gcd work 0 (by construction), of lookups 1.000
- job 6: entry share L1 0.165; L1-opt 0.143; ctl 0.165; fmpq 0.160; mpq 0.131; L1 store share of gcd work 0 (by construction), of lookups 1.000; L1-opt store share of gcd work 0 (by construction), of lookups 1.000
- job 7: entry share L1 0.199; L1-opt 0.178; ctl 0.182; fmpq 0.174; mpq 0.158; L1 store share of gcd work 0 (by construction), of lookups 1.000; L1-opt store share of gcd work 0 (by construction), of lookups 1.000
- job 8: entry share L1 0.140; L1-opt 0.143; ctl 0.151; fmpq 0.148; mpq 0.109; fz 0.036; L1 store share of gcd work 0 (by construction), of lookups 1.000; L1-opt store share of gcd work 0 (by construction), of lookups 1.000
- job 9: entry share L1 0.124; L1-opt 0.118; ctl 0.117; fmpq 0.109; mpq 0.085; L1 store share of gcd work 0 (by construction), of lookups 1.000; L1-opt store share of gcd work 0 (by construction), of lookups 1.000
- job 10: entry share L1 0.238; L1-opt 0.224; ctl 0.274; fmpq 0.260; mpq 0.252; L1 store share of gcd work 0 (by construction), of lookups 1.000; L1-opt store share of gcd work 0 (by construction), of lookups 1.000
- job 11: entry share L1 0.236; L1-opt 0.225; ctl 0.270; fmpq 0.251; mpq 0.258; L1 store share of gcd work 0 (by construction), of lookups 1.000; L1-opt store share of gcd work 0 (by construction), of lookups 1.000

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

- 1:fmpq: form A (median A 1.699 ms, B 1.721 ms)
- 1:mpq: form B (median A 4.244 ms, B 3.905 ms)
- 2:fmpq: form B (median A 1.464 ms, B 1.462 ms)
- 2:mpq: form B (median A 4.312 ms, B 4.041 ms)
- 3:fmpq: form B (median A 1.506 ms, B 1.488 ms)
- 3:mpq: form B (median A 4.344 ms, B 4.081 ms)
- 4:fmpq: form B (median A 2.159 ms, B 2.156 ms)
- 4:mpq: form B (median A 4.806 ms, B 4.361 ms)
- 5:fmpq: form A (median A 2.652 ms, B 2.673 ms)
- 5:mpq: form B (median A 4.847 ms, B 4.544 ms)
- 6:fmpq: form B (median A 2.791 ms, B 2.772 ms)
- 6:mpq: form B (median A 5.142 ms, B 4.974 ms)
- 7:fmpq: form B (median A 2.515 ms, B 2.497 ms)
- 7:mpq: form B (median A 4.246 ms, B 3.890 ms)
- 8:fmpq: form B (median A 2.197 ms, B 2.131 ms)
- 8:mpq: form B (median A 6.348 ms, B 5.709 ms)
- 9:fmpq: form A (median A 2.750 ms, B 2.751 ms)
- 9:mpq: form B (median A 6.535 ms, B 6.140 ms)
- 10:fmpq: form B (median A 0.778 ms, B 0.765 ms)
- 10:mpq: form B (median A 1.979 ms, B 1.763 ms)
- 11:fmpq: form B (median A 0.796 ms, B 0.793 ms)
- 11:mpq: form B (median A 1.897 ms, B 1.666 ms)

## Limitations (rule, Limitations)

- **Keeping every result** (spec §10 item 3): every row stores every result as a new element; on the chain jobs (8-11) this is not the libraries' normal use, so it is noted beside every chain verdict against fmpq, mpq and the best library.
- **One row with every trick:** L1-opt is timed with all its adopted tricks on together; the counters show each trick's effect on the work, but no trick's effect on time is measured alone (rule, Limitations: one row with every trick).

