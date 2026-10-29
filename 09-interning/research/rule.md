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
