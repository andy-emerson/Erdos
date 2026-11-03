# 12-integers: the Nim build of spec v0.1

Oct 30, 2026. [Spec v0.1](v0.1.md), built in Nim with its test harness. Nothing is committed, and the real timed round
has not been run.

## Files

| File | What |
| --- | --- |
| `src/control.nim` | **Control**: Nim's `uint64` and its operators, `std/math.gcd` |
| `src/pow2.nim` | **Power-of-2 flag**: bit 63, used in multiply, exact division and gcd |
| `src/even.nim` | **Even flag**: bit 63, used only in gcd's shared-power step |
| `src/bingcd.nim` | `std/math.gcd`'s binary gcd, copied so that the flag builds are self-contained |
| `src/growing.nim` | **Growing**: schoolbook words; add, subtract, compare, shifts, multiply (`unsigned __int128`), Hensel exact division, binary gcd; `-d:W=64` (default) or `32` |
| `src/bytes.nim` | **Byte encoding**: LEB128 length, then L bytes, little-endian. Each operation does 2 decodes, Growing's routine and 1 encode |
| `src/bits.nim`, `src/alloc.nim` | gcc's ctz/clz builtins; `malloc`/`free`, with byte accounting in the memory build |
| `src/bench.nim` | the driver (not a build): modes `dump`, `calib`, `time`, `mem` |
| `build.sh` | builds 17 binaries into `build/bin/` |
| `test/gen.py` | the operands (§3): one `random.Random(20261030)`, all jobs in order |
| `test/check.py` | the correctness check |
| `test/run.py` | the timing harness (§4) and its analysis |

## Build, check, run

```sh
NIM=/path/to/nim-2.2.14/bin/nim sh build.sh   # Nim 2.2.14 as pinned in ../11-matrix/nim/README.md; gcc 13.3.0
python3 test/gen.py && python3 test/check.py  # the correctness check (build/work, build/check)
python3 test/run.py --toy                     # toy run: N = 32, 3 reps of 2 ms; build/toy/report.md
python3 test/run.py                           # the real round: quiet machine and the author's go-ahead only
```

The flags are the ones from 11-matrix: `-d:danger --mm:arc -d:useMalloc --panics:on`, gcc `-O2 -fno-strict-aliasing`.
Each build has these binaries:
- `<b>-j1`, used for job 1. It is scalar (`-fno-tree-vectorize`).
- `<b>`, used for jobs 2–16.
- `<b>-mem`, the memory build.
- `growing-w32` and `bytes-w32`, used only by the check.

## Harness

`run.py` uses the coordinator's method:
- **Setup:** it writes the job files and runs the correctness check as a gate.
- **Space and instructions:** the memory build gives bytes per value (accounted, with heap beside) and the accounted peak. Callgrind gives Ir per operation, counted only in `loop_<op>`, as the run with 2 passes less the run with 1, divided by N.
- **Timed blocks:** there is one block per job × operation. Before each block the runner waits until the machine is idle, meaning CPU at most 10% over a 1 s sample and 1-minute load at most 1.5, re-checking every 10 s. Each row runs pinned with `taskset -c 1`. It is calibrated so that one rep (r passes over the operands) takes 50–100 ms, then runs 15 reps. The runner reports the median time per operation and the spread (max − min) / median.
- **Rows and noise:** the five builds run as rows, with Control again as a sixth row (`control-2`). The noise is |Control − Control-2| / the smaller of the two. A difference counts only if T(ref)/T(row) lies beyond 1 ± max(5%, noise).
- **Pairings:** job 1, each build × Control; jobs 2–16, Byte encoding × Growing; within each build, absolute difference × subtract (using the larger noise of the two blocks).
- **Checksum:** the binary stores every result and checksums it after the clock.
- **Output:** `build/<toy|results>/report.md` and `results.json`.

## Builder's readings (for the author to confirm)

1. **Operands wider than the word.** Control takes each operand mod 2^64, and the flag builds take it mod 2^63. This is the spec's "wraps", so most results in jobs 2–16 are inexact, as counted.
2. **Size class.** For k words, bit lengths run uniformly from 64(k−1)+1 to 64k. Job 1 runs from 1 to 63.
3. **Half size.** n is drawn from the class.
   - Exact division: b has max(1, ⌊n/2⌋) bits and q has max(1, n − ⌊n/2⌋) bits.
   - gcd: g has max(1, ⌊n/2⌋) bits, and x and y have max(1, n − ⌊n/2⌋) bits each.
4. **Pairs.** Subtract and absolute difference use add's pairs, each in random order, in the same order for both. Multiply's pairs are drawn separately.
5. **Powers of 2.** Each drawn quantity of l bits is 2^(l−1) with probability 1/16, which is the spec's placeholder.
6. **Subtract with a < b in Growing.** The result wraps mod W^max(len a, len b), because schoolbook subtraction drops the final borrow.
7. **Space measures.**
   - Bytes per value is the mean over the cell's operands and results.
   - Peak is the most the operation itself holds at once, its result included.
   - Growing's multiply and add allocate la+lb and max+1 words. The accounted bytes are what was allocated.

## Findings

1. **Exact division traps where a divisor wraps to 0.** A power-of-2 divisor wider than the build's width wraps to 0 in Control and in both flag builds. The hardware divide then raises SIGFPE and the binary dies. This affects 11 cells:
   - Control, jobs 4, 8 and 16: 66, 61 and 71 of 1024 divisors are 0;
   - Power-of-2 and Even flag, job 2: 2 divisors are 0;
   - Power-of-2 and Even flag, jobs 4, 8 and 16: 66, 61 and 71 divisors are 0.

   Preventing the trap would need a special case that the spec forbids, so the harness records these cells as "trap" and gives them no time. This conflicts with "every operation on every build, no exceptions" and **needs the author's decision before the real round**.
2. **The L1 rule holds only for small jobs.** With 1024 operations, the operand arrays fit the 48 KiB L1d only for k ≤ 2. At k = 16 they take about 280 KB.

## Correctness (`check.py`, Oct 30, 2026)

The check covered 210 cells: five builds × six operations × five jobs, with Growing and Byte encoding at W = 32 besides. It checked 203,776 results against Python's exact values and against each build's model:
- **Failures:** none.
- **Traps:** the 11 cells listed above.
- **Growing and Byte encoding:** every result is exact except subtract with a < b (546, 535, 510, 523 and 511 by job), and those match the wrap. The same holds at W = 32.
- **Control and the flag builds:** in job 1, the inexact results are subtract with a < b (546) and multiply overflow (497 Control, 513 flag builds); each flag build also has 2 inexact adds. In jobs 2–16 nearly every result is inexact, as the operands wrap. Every one matches the build's model and every flag is correct.
