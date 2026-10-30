# Round m3: where the instructions and bytes go (analysis, not a result)

Oct 30, 2026. Analysis only. No project code changed, nothing committed, no wall-clock timing.

## Method

- **Tool.** `valgrind --tool=callgrind` (Ir = instructions executed) on the harness in `time` mode, `r = 1`, one rep, one run at a time. 48 runs in all: 4 models × 4 ops × jobs **1, 6, 10** (n = 16, 32, 64; the pilot jobs, present in every set).
- **Binaries.** The timing-build command lines from `m3_libconf.build_commands`, rebuilt into the scratchpad with `-g` added. The `.text` of every object is byte-identical to `test/build/m3/bin/obj/timing` (checked with sha256), so the code profiled is the code measured.
- **Splitting the run.** `--dump-before=<row>_*ea*` (create, read) splits each run into parts:
  - **build:** each input's create and its enters, averaged per input;
  - **op:** the result's create, through madd/mmul, up to the first read. This is the harness's timed span; it includes freeing sop/pos intermediates.
- **Phases.** Each instruction is assigned by its compiled function first, and by its inlined source function where that is more specific (e.g. `widen` or `probe` inlined into `store_whole`).
- **Counters.** The trial-division and refinement counts come from `results-m3-*/factoring.csv`. Bit sizes come from the `.ref` and `.wl` files. A closed-form trial-division count, recomputed from `sympy.factorint`, matches `td_cand` exactly (e.g. 1,128,946,966 for mul job 9).
- **Ir and time are not the same thing.** At n = 64, (Ir ratio vs TC) against (measured time ratio vs TC):

  | Row and measure | Ir ratio | Time ratio |
  | --- | --- | --- |
  | Hash-V-pack build | 2.48 | 1.72 |
  | Hash-V-pack add | 3.11 | 2.15 |
  | Hash-V-pack mul | 1.31 | 1.12 |
  | Pack-primes mul | 64.5 | 106 |
  | Pack-primes sop | 95 | 158 |
  | Pack-coprime add | 499 | 566 |
  | Pack-coprime mul | 21.2 | 20.2 |

  - Hash-V-pack's extra work is cheap, high-IPC code (switches, streaming copies), so Ir overstates its time.
  - Trial division costs one 64-bit `div` per candidate, so Ir understates it by about 1.65×.
  - Pack-coprime is gcd-bound, like TC, so its Ir and time ratios agree.
  - **Read the percentages as instruction shares. Division-heavy phases weigh more in time.**

## Split by phase (% of Ir, n = 64, job 10; build = one input of the mul set)

Phase key:
- **gcd:** binary gcd (`sk_gcd_raw`).
- **reads+cmp:** operand decode (handles, width-switched `slot_get`/`fld_get`, `rd1`) and the value tests in `compare_*`.
- **cases:** the case logic of add_body / mul_cells / tcm.
- **hash:** `store_whole` probing, the table's create/init and its widening.
- **cellst:** writing handle fields.
- **pack:** pack.
- **TD:** trial division.
- **cop1:** coprime pass 1 (the gcds, plus about 7–12 points of reading pieces).
- **cop2:** pass-2 divisibility tests.
- **flist:** writing (f), `fbuf_put`.
- **fgcd:** merges and gcds from exponent lists.
- **recon:** reconciliation.
- **alloc:** malloc/free.

"·" means under 0.5%.

| model | what | Ir | ×TC Ir, n=16/32/64 | gcd | arith | reads+cmp | cases | hash | cellst | pack | TD | cop1 | cop2 | flist | fgcd | recon | alloc | other |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| TC | build | 0.73M | 1 | 66 | 5 | · | 21 | · | 1 | · | · | · | · | · | · | · | · | 7¹ |
| TC | add | 1.04M | 1 | 47 | 16 | 7 | 28 | · | 1 | · | · | · | · | · | · | · | · | · |
| TC | mul | 213M | 1 | 71 | 9 | 3 | 17 | · | 1 | · | · | · | · | · | · | · | · | · |
| TC | sop | 431M | 1 | 71 | 9 | 3 | 17 | · | 1 | · | · | · | · | · | · | · | · | · |
| TC | pos | 245M | 1 | 74 | 8 | 3 | 15 | · | 1 | · | · | · | · | · | · | · | · | · |
| Hash-V-pack | build | 1.82M | 2.66/2.53/2.48 | 26 | 2 | · | 10 | 27 | 11 | 20 | · | · | · | · | · | · | · | 3¹ |
| Hash-V-pack | add | 3.24M | 3.25/3.26/3.11 | 15 | 5 | 27 | 13 | 22 | 7 | 11 | · | · | · | · | · | · | · | · |
| Hash-V-pack | mul | 278M | 1.34/1.32/1.31 | 54 | 7 | 23 | 15 | · | 1 | · | · | · | · | · | · | · | · | · |
| Hash-V-pack | sop | 563M | 1.36/1.33/1.31 | 54 | 7 | 23 | 15 | · | 1 | · | · | · | · | · | · | · | · | · |
| Hash-V-pack | pos | 312M | 1.35/1.31/1.27 | 57 | 7 | 20 | 14 | 1 | 1 | · | · | · | · | · | · | · | · | · |
| Pack-primes | build | 2.22M | 5.1/3.13/3.04 | 22 | 1 | · | 8 | 22 | 9 | 17 | 13 | · | · | 5 | · | · | · | 2¹ |
| Pack-primes | add | 7.09M | 7.13/7.10/6.82 | · | 3 | 13 | 9 | 10 | 3 | 5 | **42** | · | · | 6 | 9 | · | · | · |
| Pack-primes | mul | 13.7G | 97/78/65 | · | · | · | 1 | · | · | · | **97** | · | · | · | 1 | · | · | · |
| Pack-primes | sop | 41.1G | 177/161/95 | · | · | · | 1 | · | · | · | **98** | · | · | · | 1 | · | · | · |
| Pack-primes | pos | 22.4G | 230/120/91 | · | · | · | 1 | · | · | · | **98** | · | · | · | 1 | · | · | · |
| Pack-coprime | build | 29.7M | 84/23/41 | 2 | · | · | 1 | 2 | 1 | 1 | · | **89** | 5 | · | · | · | · | · |
| Pack-coprime | add | 519M | 95/218/499 | · | · | · | · | · | · | · | · | **95** | 4 | · | · | ·² | · | · |
| Pack-coprime | mul | 4.50G | 7.7/12.3/21.2 | · | 1 | · | 3 | · | · | · | · | **89** | 2 | · | 5 | · | · | · |
| Pack-coprime | sop | 13.2G | 10.5/17.6/30.7 | · | · | · | 2 | · | · | · | · | **92** | 2 | · | 4 | · | · | · |
| Pack-coprime | pos | 5.82G | 10.2/14.8/23.8 | · | 1 | · | 2 | · | · | · | · | **89** | 2 | · | 5 | · | · | · |

¹ The harness's own enter loop (`run`), inside the timed build. It is the same absolute cost in every row.
² Reconciliation is 2.8% at n = 16 and 0.6–2.1% for the products at n ≤ 32. It is never above 3%.

Across n, the shares are stable for TC and Hash-V-pack (within ±3 points). In the factored rows, factoring's share grows with n: Pack-coprime's cop1 goes from 67–78% at n = 16 to 84–86% at n = 64.

## Answers

**1. Hash-V-pack add 2.2× vs mul 1.13×.** Per result cell at n = 64, Hash-V-pack add costs 791 Ir against TC's 254.
- **Fixed per-result cost, 323 Ir/cell** (1.27 × TC's whole add per cell):
  - 2 hash stores: 150 (each probe goes through a runtime width switch);
  - pack: 89. It scans all T = 16384 slots and rewrites the 8192 fields through the map;
  - field writes: 58;
  - widen: 16. The result table starts at 8 bits and widens to 32 bits for add, 64 bits for mul, copying all T slots each time;
  - create/init: 10.
- **Per-operation indirection, about 190 Ir/cell** (0.75×): decoding 2 handle fields plus 4 values through width-switched `fld_get`/`slot_get`, plus the 10 value comparisons of `compare_cells`. TC reads a `tpair` directly.
- **Why mul is cheaper:** mul amortises the fixed part over n = 64 multiply-adds per cell (under 1.5% of Ir). Only the indirection remains: reads+cmp is 23% of Hash-V-pack mul against 3% of TC mul. That is 57M of the 65M excess Ir, and the whole 1.13×.

**2. Hash-V-pack build 1.72×.** Same gcd and arithmetic as TC (482k vs 485k Ir per input). On top of that, per input at n = 64:
- hash probing: 364k Ir;
- pack: 367k Ir. It scans T = 16384 slots whatever W is (W = 1013 at ρ = 4: 6% fill), and rewrites 8192 fields;
- field writes: 209k Ir;
- create: 131k Ir. Each of the 16384 64-bit slots is set to EMPTY one at a time through `slot_set`'s switch.

These are cheap instructions: 2.48× in Ir, 1.72× in time.

**3. Pack-primes: trial division.** It is 97–98% of mul/sop/pos Ir and 42% of add Ir. With the extra-latency `div`, its share of time is even higher. It costs about 11 Ir per candidate (each candidate one 64-bit `div`).

| Numbers trial-divided | Bits, median (max) | Candidates per number | Matrix total |
| --- | --- | --- | --- |
| Inputs | 13 (15) | about 22 | 13% of a build |
| add results | 16 (24) | 65 | 0.26M per matrix |
| mul result numerators | 43 (49) | 270k | 1.13G per matrix |
| pos result numerators | 45 (50) | 459k | 1.92G per matrix |
| product denominators (smooth) | 17 (19) | negligible | — |

- **The cost is set by the largest prime factor.** In the most expensive 10% of numbers, the largest prime factor is a median of 43 bits. That 10% does 72% of all candidates, and the top 1% does 16–20%.
- **Pack-primes needs no gcd at all on the products.** Its gcd values come from merges (fgcd). That merge path costs 192M Ir on mul n = 64, against TC's whole binary-gcd bill of 151M, so factored gcds do not save instructions even before factoring is counted.

**4. Pack-coprime: eager refinement.**
- **How much it is:** pass 1 (gcds and piece reads) plus pass 2 is 99% of add Ir at n = 64 (95% at n = 16). Reconciliation, the on-demand refinement, is at most 3%.
- **Why it costs so much:** pass 1 scans the base linearly for every number. A number with a new prime piece scans the whole base, because new pieces are appended at the end and the scan stops only when `rest = 1`. So pass 1 costs about 0.3·W·nq gcds per matrix, and pass 2 repeats the same scan as divisions. At n = 64:

  | Op | W | nq | pass-1 gcds | Ir per gcd |
  | --- | --- | --- | --- | --- |
  | add | 3988 | 1916 | 2.46M | about 180 (31-bit numbers) |
  | mul | 4189 | 4560 | 9.4M | about 400 (43-bit) |

- **Why add explodes and mul doesn't:** the factoring is about n⁴ (W ~ n², nq ~ n²) for both operations. The denominators differ: TC add is n² operations (1.04M Ir), while TC mul is n³ (213M Ir). Add's factoring is 8.7× cheaper than mul's in absolute terms, but TC's add is 204× cheaper than TC's mul. Hence 499× vs 21× in Ir (566× vs 20× in time).
- **Inherent or not:** this is inherent to eagerly factoring every result, but the quadratic cost comes from the linear-scan algorithm. Possible fixes: an index from pieces, batch gcd or a product tree, or recording each number's pieces in pass 1 so that pass 2 is not needed.

**5. Peak during mul, Hash-V-pack 1.30×** (job 10: 262.6 KB against TC's 196.6 KB, matching 21.38 B/cell × 3n²).
- Peak is at the result's pack, when the following are all alive:
  - the inputs, packed: 2 × 24.4 KB (16-bit cells 16 KB + list 8 KB);
  - **(h), 128 KB:** T = 16384 slots × 8 B. The table is sized for 2n² + 2 → 16384 and is widened to 64 bits at the first product, which already exceeds 32 bits;
  - result cells: 16 KB;
  - (l): 33.5 KB;
  - **(p): 32 KB:** T × 2 B.
- (h) alone is 2/3 of TC's whole peak.
- The factored rows add (f), plus (q) for Pack-coprime, on top: 25.4 and 29.0 B/cell.

## Weak points, ranked (by instructions at n = 64 and how much of the gap each explains)

1. **Pack-primes, full trial division of product numerators.** 97–98% of mul/sop/pos (1–2G candidates per matrix on 43–50-bit numbers). Algorithmic: about √(largest prime) per number. Inherent to schoolbook TD, but full factoring of numerators is a model choice. Reduction needs only the denominators' {2,3,5}-smooth factors. A primality test on the cofactor plus Pollard rho, or lazy numerator factoring, removes nearly all of it.
2. **Pack-coprime, pass 1's linear base scan.** 84–92% of every op, and 89% of build. About 0.3·W·nq gcds of 180–400 Ir each. Algorithmic and an implementation choice (no index; restarts from piece 0). That is what makes add 499×.
3. **Pack-coprime, pass 2 rescans the base with divisions** that pass 1 already found. 2–5% of Ir, more in time (div latency). Implementation: record the exponents during pass 1.
4. **Hash-V-pack, handle indirection on every operand read.** Width-switched `fld_get`/`slot_get` plus 10 value compares per cell: 23% of Ir on mul/sop/pos. That is the entire 1.13× and about 40% of add's excess. Constant factor, from the implementation: specialise by width, or decode A's row i once per i (it is re-read n times in mmul).
5. **Hash-V-pack, per-result store path** (2 probes, field writes, create, widen): 36% of add Ir. Constant factor per result cell, inherent to building a hash. Cheaper hashing/switch-free slots would help, and the result table could start at the right width (mul widens at the first store).
6. **Hash-V-pack, pack scans and allocates on T, not W.** 11% of add and 20% of build. The memset-like loop and the scan cover 16384 slots, even when W = 87–1013. Implementation: (h) is sized for 2n² + 2 distinct values; a `memset` init, a compacting pass over used slots, or sizing from a W estimate would cut it.
7. **Hash-V-pack, (h) at 64 bits × T alive during the result's build.** About 50% of the mul peak, and (p) is another 12%. Model choice (worst-case table); a smaller T, or packing (l) without a full (p), lowers the 1.30×.
8. **Factored rows, gcds via exponent-list merges.** 5% of Pack-coprime mul. Costs about 1.3× TC's binary gcds in Ir, so it saves nothing even with factoring free. Implementation: list merges and lcm/sub list building per operation.
9. **Pack-primes, factoring the inputs** at build: 13% TD plus 5% (f) writes on 13-bit numbers. Small, but it doubles Pack-primes' build over Hash-V-pack at n = 16 (5.1× vs 2.7×). It would vanish with a small-prime table or a wheel.
10. **Not weak:** reconciliation (≤3%) and allocation (<1.5% everywhere) do not matter on this workload. That is partly because the denominators are {2,3,5}-smooth.

## Limits

- Ir only, one run per configuration.
- The build split uses the mul set's job 10 (ρ = 4); ρ = 1 inputs are larger: Pack-coprime build is 84× TC at job 1.
- The pass-1/pass-2 boundary inside `factor` is by source line. Inlined helpers (`slot_get`, `sk_mag`) are counted with pass 1 (about 7–12 points).
- Scratch tooling (`phases.py`, `batch.py`, `table.py`, per-config JSON): `/tmp/claude-0/-home-claude/6c273ef9-4bf4-5ea2-bd9e-566136af1776/scratchpad/`. Not in the repo.
